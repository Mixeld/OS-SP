#include "common.h"
#include <string.h>

// Убить последнего производителя
void kill_last_producer(pid_t *prods, int *p_cnt) {
    if (*p_cnt == 0) {
        printf("Error: No producers to kill\n");
        return;
    }
    
    pid_t pid = prods[*p_cnt - 1];
    printf("Killing last producer (PID: %d)\n", pid);
    kill(pid, SIGTERM);
    
    waitpid(pid, NULL, 0);
    
    (*p_cnt)--;
    printf("Last producer terminated. Remaining producers: %d\n", *p_cnt);
}

// Убить последнего потребителя
void kill_last_consumer(pid_t *cons, int *c_cnt) {
    if (*c_cnt == 0) {
        printf("Error: No consumers to kill\n");
        return;
    }
    
    pid_t pid = cons[*c_cnt - 1];
    printf("Killing last consumer (PID: %d)\n", pid);
    kill(pid, SIGTERM);
    
    waitpid(pid, NULL, 0);
    
    (*c_cnt)--;
    printf("Last consumer terminated. Remaining consumers: %d\n", *c_cnt);
}

int main() {
    // Сброс флага keep_running перед запуском
    reset_keep_running();
    
    // Установка обработчика Ctrl+C для родительского процесса
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigterm);
    
    // Создание разделяемой памяти для очереди
    int shmid = create_shared_memory(sizeof(SharedQueue));
    SharedQueue *q = (SharedQueue*)attach_shared_memory(shmid);
    memset(q, 0, sizeof(SharedQueue));

    // Создание и инициализация семафоров
    int semid = create_semaphores(3);
    init_semaphores(semid);

    // Массивы для хранения PID дочерних процессов
    pid_t prods[MAX_PROC], cons[MAX_PROC];
    int p_cnt = 0, c_cnt = 0;

    printf("\nCommands:\n");
    printf("  p        - add producer\n");
    printf("  c        - add consumer\n");
    printf("  kpl      - kill last producer\n");
    printf("  kcl      - kill last consumer\n");
    printf("  s        - show status\n");
    printf("  q        - quit\n");
    printf("Press Ctrl+C for graceful shutdown\n");
    printf("> ");
    fflush(stdout);

    char cmd[10];
    
    while (keep_running) {
        memset(cmd, 0, sizeof(cmd));
        
        int ret = scanf("%9s", cmd);
        if (ret == EOF) {
            break;
        }
        
        if (strcmp(cmd, "p") == 0 && p_cnt < MAX_PROC) {
            pid_t p = fork();
            if (p == 0) {
                signal(SIGINT, SIG_DFL);
                run_producer(semid, q);
                exit(0);
            } else if (p > 0) {
                prods[p_cnt++] = p;
                printf("Producer %d started (PID: %d)\n", p_cnt, p);
            }
        } 
        else if (strcmp(cmd, "c") == 0 && c_cnt < MAX_PROC) {
            pid_t p = fork();
            if (p == 0) {
                signal(SIGINT, SIG_DFL);
                run_consumer(semid, q);
                exit(0);
            } else if (p > 0) {
                cons[c_cnt++] = p;
                printf("Consumer %d started (PID: %d)\n", c_cnt, p);
            }
        }
        else if (strcmp(cmd, "kpl") == 0) {
            kill_last_producer(prods, &p_cnt);
        }
        else if (strcmp(cmd, "kcl") == 0) {
            kill_last_consumer(cons, &c_cnt);
        }
        else if (strcmp(cmd, "s") == 0) {
            printf("\n--- STATUS ---\n"
                   "Queue Load: %d/%d\n"
                   "Added: %d, Extracted: %d\n"
                   "Producers: %d, Consumers: %d\n"
                   "--------------\n",
                   q->current_load, QUEUE_SIZE, 
                   q->added_count, q->extracted_count, 
                   p_cnt, c_cnt);
        }
        else if (strcmp(cmd, "q") == 0) {
            break;
        }
        else if (strlen(cmd) > 0) {
            printf("Unknown command: %s\n", cmd);
        }
        
        printf("> ");
        fflush(stdout);
    }

    if (!keep_running) {
        printf("\n[Parent] Graceful shutdown initiated...\n");
    } else {
        printf("\n[Parent] Shutting down by user request...\n");
    }
    
    // Отправка сигнала завершения всем дочерним процессам
    if (p_cnt > 0) {
        printf("[Parent] Stopping %d producers...\n", p_cnt);
        for (int i = 0; i < p_cnt; i++) {
            kill(prods[i], SIGTERM);
        }
    }
    
    if (c_cnt > 0) {
        printf("[Parent] Stopping %d consumers...\n", c_cnt);
        for (int i = 0; i < c_cnt; i++) {
            kill(cons[i], SIGTERM);
        }
    }

    // Ожидание завершения дочерних процессов
    printf("[Parent] Waiting for children to terminate...\n");
    sleep(1);
    
    // Принудительное завершение оставшихся процессов
    for (int i = 0; i < p_cnt; i++) {
        if (kill(prods[i], 0) == 0) {
            printf("[Parent] Force killing producer %d\n", prods[i]);
            kill(prods[i], SIGKILL);
            waitpid(prods[i], NULL, 0);
        }
    }
    for (int i = 0; i < c_cnt; i++) {
        if (kill(cons[i], 0) == 0) {
            printf("[Parent] Force killing consumer %d\n", cons[i]);
            kill(cons[i], SIGKILL);
            waitpid(cons[i], NULL, 0);
        }
    }

    // Освобождение IPC ресурсов
    printf("[Parent] Cleaning up IPC resources...\n");
    destroy_shared_memory(shmid, q);
    semctl(semid, 0, IPC_RMID);
    
    printf("[Parent] Done. Exiting.\n");
    return 0;
}