#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

extern char **environ;

int main(int argc, char *argv[], char *envp[]) {
    if (argc < 2) {
        fprintf(stderr, "Child error: missing arguments\n");
        return EXIT_FAILURE;
    }

    printf("\n[CHILD] Имя: %s | PID: %d | PPID: %d\n", argv[0], getpid(), getppid());

    if (argv[1][0] == '+') {
        printf("[CHILD] Вывод окружения через extern char **environ:\n");
        for (int i = 0; environ[i] != NULL; i++) {
            printf("  %s\n", environ[i]);
        }
    } 

    else if (argv[1][0] == '*') {
        printf("[CHILD] Вывод окружения через char *envp[] (параметр main):\n");
        for (int i = 0; envp[i] != NULL; i++) {
            printf("  %s\n", envp[i]);
        }
    } else {
        fprintf(stderr, "Child error: unknown action '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    printf("[CHILD] Завершение работы.\n\n");
    return EXIT_SUCCESS;
}