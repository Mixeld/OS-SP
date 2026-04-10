#include "common.h" 

static pid_t child_pids[MAX_CHILDREN];
static int child_count = 0;
static volatile pid_t ready_to_print_pid = 0;
static struct termios orig_termios;


static void disable_raw_mode(){
    tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("ГГ ТЕРМИНАЛУ\n");
}

static void enable_raw_mode(){
    tcgetattr (STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void parent_usr1_handler (int signum, siginfo_t *siginfo, void *context){
    (void)signum;
    (void)context;
    ready_to_print_pid = siginfo -> si_pid;
}

static void create_child (){
    if (child_count >= MAX_CHILDREN){
        printf ("[p] БОЛЬШЕ Я СОЗДАВАТЬ НЕ МОГУ. \n");
        return;
    }

    pid_t pid = fork(); 
    if(pid == -1){
        perror ("вилка отвалилась");
        return;
    }

    if (pid == 0) {
        printf("[C %d] FORKED SUCCESSFULLY! About to enter child_main_loop...\n", getpid());
        child_main_loop();
        exit(0);
    } else {
        child_pids[child_count++] = pid;
        printf("[p] Created child with PID = %d, Total children: %d\n", pid, child_count);
    }
}

static void list_processes(){
    printf("[p] Parent PID: %d\n", getpid());
    printf("[p] Children PIDs (%d total): ", child_count);
    for(int i = 0; i < child_count; i++){
        printf("%d ", child_pids[i]);
    }
    printf ("\n");
}

static void delete_last_child(){
    if (child_count == 0){
        printf ("NO MORE CHILDREN\n");
        return;
    }

    pid_t pid_to_kill = child_pids[--child_count];
    printf("[p] Deleting child with PID = %d\n", pid_to_kill);
    kill (pid_to_kill, SIGTERM);
    waitpid(pid_to_kill, NULL, 0);
    child_pids [child_count] = 0;
}

static void kill_all_children (){
    if (child_count == 0){
        printf ("NO MORE CHILDREN\n");
        return;
    }
    printf ("[p] kill all %d children \n", child_count);
    while(child_count > 0) {
        delete_last_child();
    }    
    printf("[p] ALL CHILDREN KILLED\n ");
}


int main (){
    enable_raw_mode();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = parent_usr1_handler;
    sa.sa_flags = SA_SIGINFO;
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("FATAL: sigaction failed");
        exit(1);
    }

    printf("[p] START!\n");
    printf("'+' - create \n'-' - delete last \n'l' - list \n'k' - kill all \n'q' - quit\n" );
    fflush(stdout);

    while (1) {
        if (ready_to_print_pid != 0) {
            printf("[P] Child %d is ready to report. Authorizing print...\n", ready_to_print_pid);
            fflush(stdout);

            kill(ready_to_print_pid, SIGUSR2);
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
            char c = getchar();
            switch (c) {
                case '+': create_child(); break;
                case '-': delete_last_child(); break;
                case 'l': list_processes(); break;
                case 'k': kill_all_children(); break;
                case 'q': 
                    printf("[p] Quitting ...\n");
                    kill_all_children();
                    exit(0);
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
    
    return 0;
}
