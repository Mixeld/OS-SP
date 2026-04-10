#include "common.h"

static volatile struct stats_data stats;
static volatile sig_atomic_t alarm_fired = 0;
static volatile sig_atomic_t can_print = 0;
static volatile sig_atomic_t terminate_child = 0;

static void child_alarm_handler(int signum) {
    (void)signum;
    alarm_fired = 1;
}

static void child_usr2_handler(int signum) {
    (void)signum;
    can_print = 1;
}

static void child_term_handler(int signum) {
    (void)signum;
    terminate_child = 1;
}

void child_main_loop(void) {

    struct sigaction sa_alarm, sa_usr2, sa_term;
    
    memset(&sa_alarm, 0, sizeof(sa_alarm));
    sa_alarm.sa_handler = child_alarm_handler;
    sigaction(SIGALRM, &sa_alarm, NULL);
    
    memset(&sa_usr2, 0, sizeof(sa_usr2));
    sa_usr2.sa_handler = child_usr2_handler;
    sigaction(SIGUSR2, &sa_usr2, NULL);
    
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = child_term_handler;
    sigaction(SIGTERM, &sa_term, NULL);
    
    signal(SIGUSR1, SIG_DFL);

    struct itimerval work_timer;
    struct itimerval stop_timer;
    
    work_timer.it_value.tv_sec     = 0;
    work_timer.it_value.tv_usec    = 500;   
    work_timer.it_interval.tv_sec  = 0;
    work_timer.it_interval.tv_usec = 500;   

    memset(&stop_timer, 0, sizeof(stop_timer));
    
    struct pair data_pair = {0, 0};
    int toggle = 0;

    while (!terminate_child) {
        stats.s00 = stats.s01 = stats.s10 = stats.s11 = 0;

        for (int i = 0; i < STAT_CYCLES && !terminate_child; ++i) {
            alarm_fired = 0; 
            
            if (setitimer(ITIMER_REAL, &work_timer, NULL) == -1) {
                perror("setitimer");
                break;
            }

            while (!alarm_fired && !terminate_child) {
                if (toggle) {
                    data_pair.a = 1;

                    if (alarm_fired) break; 
                    data_pair.b = 1;
                } else {
                    data_pair.a = 0;
                    for (volatile int j = 0; j < 50; j++); 
                    if (alarm_fired) break; 
                    data_pair.b = 0;
                }
                toggle = !toggle;
            }
            
            if (setitimer(ITIMER_REAL, &stop_timer, NULL) == -1) {
                perror("setitimer stop");
            }
            
            if (terminate_child) break;

            if      (data_pair.a == 0 && data_pair.b == 0) stats.s00++;
            else if (data_pair.a == 0 && data_pair.b == 1) stats.s01++;
            else if (data_pair.a == 1 && data_pair.b == 0) stats.s10++;
            else if (data_pair.a == 1 && data_pair.b == 1) stats.s11++;
        }

        if (terminate_child) break;

        if (kill(getppid(), SIGUSR1) == -1) {
            perror("kill SIGUSR1 failed");
            break;
        }

        can_print = 0;
        while (!can_print && !terminate_child) {
            pause();
        }

        if (terminate_child) break;
        
        printf("[C] PPID=%d, PID=%d, stats: {0,0}=%d, {0,1}=%d, {1,0}=%d, {1,1}=%d\n",
               getppid(), getpid(), stats.s00, stats.s01, stats.s10, stats.s11);
        fflush(stdout);
    }
    
    printf("[C %d] Terminating...\n", getpid());
    exit(0);
}
