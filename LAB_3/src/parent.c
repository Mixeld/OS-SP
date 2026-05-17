#include "common.h"

static pid_t child_pids[MAX_CHILDREN];
static int child_count = 0;
static volatile sig_atomic_t ready_to_print_pid = 0;
static volatile sig_atomic_t got_sigint = 0;
static struct termios orig_termios;

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("ТЕРМИНАЛ\n");
}

static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void parent_usr1_handler(int signum, siginfo_t *siginfo, void *context) {
    (void)signum;
    (void)context;
    ready_to_print_pid = (sig_atomic_t)siginfo->si_pid;
}

static void parent_sigint_handler(int signum) {
    (void)signum;
    got_sigint = 1;
}

static void create_child(void) {
    if (child_count >= MAX_CHILDREN) {
        printf("[p] БОЛЬШЕ Я СОЗДАВАТЬ НЕ МОГУ.\n");
        return;
    }

    /*
     * Блокируем SIGINT на время fork(), чтобы ребёнок не успел
     * унаследовать и получить Ctrl+C с родительским обработчиком.
     */
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGINT);

    if (sigprocmask(SIG_BLOCK, &block_set, &old_set) == -1) {
        perror("sigprocmask");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("вилка отвалилась");
        sigprocmask(SIG_SETMASK, &old_set, NULL);
        return;
    }

    if (pid == 0) {
        /*
         * В дочернем процессе Ctrl+C игнорируем:
         * им управляет родитель.
         */
        struct sigaction sa_ignore;
        memset(&sa_ignore, 0, sizeof(sa_ignore));
        sa_ignore.sa_handler = SIG_IGN;
        sigemptyset(&sa_ignore.sa_mask);

        if (sigaction(SIGINT, &sa_ignore, NULL) == -1) {
            perror("child sigaction(SIGINT)");
            _exit(1);
        }

        sigprocmask(SIG_SETMASK, &old_set, NULL);

        printf("[C %d] FORKED SUCCESSFULLY! About to enter child_main_loop...\n", getpid());
        fflush(stdout);

        child_main_loop();

        fflush(stdout);
        _exit(0);
    } else {
        child_pids[child_count++] = pid;
        sigprocmask(SIG_SETMASK, &old_set, NULL);
        printf("[p] Created child with PID = %d, Total children: %d\n", pid, child_count);
    }
}

static void list_processes(void) {
    printf("[p] Parent PID: %d\n", getpid());
    printf("[p] Children PIDs (%d total): ", child_count);
    for (int i = 0; i < child_count; i++) {
        printf("%d ", child_pids[i]);
    }
    printf("\n");
}

static void delete_last_child(void) {
    if (child_count == 0) {
        printf("NO MORE CHILDREN\n");
        return;
    }

    pid_t pid_to_kill = child_pids[--child_count];
    printf("[p] Deleting child with PID = %d\n", pid_to_kill);

    if (kill(pid_to_kill, SIGTERM) == -1 && errno != ESRCH) {
        perror("kill");
    }

    if (waitpid(pid_to_kill, NULL, 0) == -1 && errno != ECHILD) {
        perror("waitpid");
    }

    child_pids[child_count] = 0;
}

static void kill_all_children(void) {
    if (child_count == 0) {
        printf("NO MORE CHILDREN\n");
        return;
    }

    printf("[p] kill all %d children\n", child_count);
    while (child_count > 0) {
        delete_last_child();
    }
    printf("[p] ALL CHILDREN KILLED\n");
}

static void cleanup_and_exit(int status) {
    kill_all_children();
    exit(status);
}

int main(void) {
    enable_raw_mode();

    struct sigaction sa_usr1;
    memset(&sa_usr1, 0, sizeof(sa_usr1));
    sa_usr1.sa_sigaction = parent_usr1_handler;
    sa_usr1.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_usr1.sa_mask);

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("FATAL: sigaction(SIGUSR1) failed");
        exit(1);
    }

    struct sigaction sa_int;
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = parent_sigint_handler;
    sigemptyset(&sa_int.sa_mask);

    /*
    SA_RESTART не ставим:
    тогда select() прервётся по EINTR,
    и цикл быстро увидит got_sigint.
     */
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("FATAL: sigaction(SIGINT) failed");
        exit(1);
    }

    printf("[p] START!\n");
    printf("'+' - create\n'-' - delete last\n'l' - list\n'k' - kill all\n'q' - quit\n");
    fflush(stdout);

    while (1) {
        if (got_sigint) {
            printf("\n[p] Caught Ctrl+C. Shutting down...\n");
            fflush(stdout);
            cleanup_and_exit(0);
        }

        if (ready_to_print_pid != 0) {
            pid_t pid = (pid_t)ready_to_print_pid;
            printf("[P] Child %d is ready to report. Authorizing print...\n", pid);
            fflush(stdout);

            kill(pid, SIGUSR2);
            ready_to_print_pid = 0;
        }

        fd_set fds;
        struct timeval tv;
        int ret;

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

        if (ret > 0) {
            int c = getchar();
            if (c == EOF) {
                continue;
            }

            switch (c) {
                case '+':
                    create_child();
                    break;
                case '-':
                    delete_last_child();
                    break;
                case 'l':
                    list_processes();
                    break;
                case 'k':
                    kill_all_children();
                    break;
                case 'q':
                    printf("[p] Quitting...\n");
                    fflush(stdout);
                    cleanup_and_exit(0);
                    break;
            }
            fflush(stdout);
        } else if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("select failed");
            break;
        }
    }

    cleanup_and_exit(1);
    return 0;
}