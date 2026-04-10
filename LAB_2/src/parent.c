#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>

extern char **environ;

int compare_env (const void *a, const void *b) {
    return strcmp(*(const char * *)a, *(const char * *)b);
}

void sigchld_handler(int signum) {
    (void)signum; 
    int saved_errno = 1; 
    while (waitpid(-1, NULL, WNOHANG) > 0);
    (void)saved_errno;
}

int main (void){
    struct sigaction sa;
    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        perror ("signaction error");
        return EXIT_FAILURE;
    }

    setenv ("LC_COLLATE", "C", 1);

    int env_count = 0;
    while (environ[env_count] != NULL) env_count++;

    char **env_copy = malloc(env_count * sizeof(char *));
    if (!env_copy) {
        perror("malloc error (env_copy)");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < env_count; i++){
        env_copy[i] = environ[i];
    }

    qsort (env_copy, env_count, sizeof(char *), compare_env);

    printf (" РОДИТЕЛЬСКОЕ ОКРУЖЕНИЕ ");

    for (int i = 0; i < env_count; i++){
        printf("%s\n", env_copy[i]);
    }

    free(env_copy);

    FILE *f  = fopen ("env", "r");

    if(!f) {
        perror("ошибка с env");
        return EXIT_FAILURE;
    }

    int c_env_capacity = 10;
    int c_env_count = 0;
    char **child_env = malloc(c_env_capacity * sizeof(char *));
    if(!child_env){
        perror("Память течёт!!!!!");
        fclose(f);
        return EXIT_FAILURE;
    }

     char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0; 
        if (strlen(line) == 0) continue;

        char *val = getenv(line);
        if (val) {
            if (c_env_count >= c_env_capacity - 1) {
                c_env_capacity *= 2;
                char **tmp = realloc(child_env, c_env_capacity * sizeof(char *));
                if (!tmp) {
                    perror("realloc error");
                    break;
                }
                child_env = tmp;
            }

            size_t len = strlen(line) + strlen(val) + 2; 
            char *new_var = malloc(len);
            if (new_var) {
                snprintf(new_var, len, "%s=%s", line, val);
                child_env[c_env_count++] = new_var;
            }
        }
    }

    child_env[c_env_count] = NULL;
    fclose(f);

    char child_path[PATH_MAX];
    char *child_path_dir = getenv("CHILD_PATH");
    if (child_path_dir && strlen(child_path_dir) > 0) {
        snprintf(child_path, sizeof(child_path), "%s/child", child_path_dir);
    } else {
        snprintf(child_path, sizeof(child_path), "./child");
    }

    int child_num = 0;
    char action;
    printf("Введите '+' (environ), '*' (envp) или 'q' (выход): \n");

    while (scanf(" %c", &action) == 1) {
        if (action == '+' || action == '*') {
            pid_t pid = fork();
            
            if (pid < 0) {
                perror("fork error");
            } else if (pid == 0) {
                sa.sa_handler = SIG_DFL;
                sigaction(SIGCHLD, &sa, NULL);

                char child_name[16];
                snprintf(child_name, sizeof(child_name), "child_%02d", child_num);
                
                char action_str[2] = {action, '\0'};
                char *child_argv[] = {child_name, action_str, NULL};

                execve(child_path, child_argv, child_env);
                
                perror("execve error (проверь пути и права)");
                exit(EXIT_FAILURE);
            } else {
                child_num = (child_num + 1) % 100; 
            }
        } else if (action == 'q') {
            printf("\nОжидание завершения оставшихся дочерних процессов...\n");
            
            sa.sa_handler = SIG_DFL;
            sigaction(SIGCHLD, &sa, NULL);
            
            while (wait(NULL) > 0);
            printf("Выход\n");
            break;
        } else {
            printf("Неверный ввод. Жми +, * или q.\n");
        }
    }

    for (int i = 0; i < c_env_capacity; i++){
        free(child_env[i]);
    }

    free(child_env);

    return EXIT_SUCCESS;

}