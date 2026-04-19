#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>      // printf, getchar, stdout, fflush
#include <stdlib.h>     // atexit, exit
#include <unistd.h>     // fork, getpid, getppid, pause, STDIN_FILENO
#include <signal.h>     // signal, sigaction, kill, sig_atomic_t, SIG*
#include <sys/time.h>   // setitimer, struct itimerval
#include <sys/wait.h>   // waitpid
#include <termios.h>    // struct termios, tcgetattr, tcsetattr
#include <string.h>     // memset
#include <errno.h>      // errno 

#define MAX_CHILDREN 100
#define STAT_CYCLES 2000 
#define MAX_CYCLES 50

struct pair {
    int a;
    int b;
};

struct stats_data{
    int s00;
    int s01;
    int s10;
    int s11;
};

void child_main_loop(void);

#endif
