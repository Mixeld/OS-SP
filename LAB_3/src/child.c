#include "common.h"
#include <stdlib.h> // <--- Для rand() и srand()
#include <time.h>   // <--- Для time()

static volatile struct stats_data stats;
static volatile sig_atomic_t can_print = 0;
static volatile sig_atomic_t terminate_child = 0;

static void child_usr2_handler(int signum) {
    (void)signum;
    can_print = 1;
}

static void child_term_handler(int signum) {
    (void)signum;
    terminate_child = 1;
}

void child_main_loop(void) {

    struct sigaction sa_usr2, sa_term;
    
    memset(&sa_usr2, 0, sizeof(sa_usr2));
    sa_usr2.sa_handler = child_usr2_handler;
    sigaction(SIGUSR2, &sa_usr2, NULL);
    
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = child_term_handler;
    sigaction(SIGTERM, &sa_term, NULL);
    
    signal(SIGUSR1, SIG_DFL);

    srand(time(NULL) ^ getpid());

    int cycles_completed = 0;  

    while (!terminate_child && cycles_completed < MAX_CYCLES) {
        
        stats.s00 = stats.s01 = stats.s10 = stats.s11 = 0;

        for (int i = 0; i < STAT_CYCLES && !terminate_child; ++i) {
            
            int random_choice = rand() % 4;

            switch (random_choice) {
                case 0: stats.s00++; break;
                case 1: stats.s01++; break;
                case 2: stats.s10++; break;
                case 3: stats.s11++; break;
            }
        }

        if (terminate_child) break;

        cycles_completed++;
        printf("[C %d] Cycle %d/%d completed. Waiting for parent permission...\n", 
               getpid(), cycles_completed, MAX_CYCLES);

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

        if (cycles_completed < MAX_CYCLES && !terminate_child) {
            printf("[C %d] Sleeping 2 seconds before next cycle...\n", getpid());
            sleep(2);
        }
    }
    
    if (cycles_completed >= MAX_CYCLES) {
        printf("[C %d] Reached maximum cycles (%d). Terminating gracefully...\n", 
               getpid(), MAX_CYCLES);
    } else {
        printf("[C %d] Terminated by signal.\n", getpid());
    }
    
    exit(0);
}