#include "common.h"
#include <string.h>
#include <sys/select.h>

SharedQueue *global_queue = NULL;

// Убить последнего производителя
void kill_last_producer(void) {
    if (p_cnt == 0) {
        printf("Error: No producers to kill\n");
        return;
    }
    
    int last_idx = -1;
    for (int i = p_cnt - 1; i >= 0; i--) {
        if (producers[i].active) {
            last_idx = i;
            break;
        }
    }
    
    if (last_idx == -1) {
        printf("Error: No active producers found\n");
        return;
    }
    
    printf("Killing last producer (Thread ID: %lu)\n", producers[last_idx].thread);
    producers[last_idx].active = 0;
    pthread_cancel(producers[last_idx].thread);
    pthread_join(producers[last_idx].thread, NULL);
    
    for (int i = last_idx; i < p_cnt - 1; i++) {
        producers[i] = producers[i + 1];
    }
    p_cnt--;
    
    printf("Last producer terminated. Remaining producers: %d\n", p_cnt);
}

// Убить последнего потребителя
void kill_last_consumer(void) {
    if (c_cnt == 0) {
        printf("Error: No consumers to kill\n");
        return;
    }
    
    int last_idx = -1;
    for (int i = c_cnt - 1; i >= 0; i--) {
        if (consumers[i].active) {
            last_idx = i;
            break;
        }
    }
    
    if (last_idx == -1) {
        printf("Error: No active consumers found\n");
        return;
    }
    
    printf("Killing last consumer (Thread ID: %lu)\n", consumers[last_idx].thread);
    consumers[last_idx].active = 0;
    pthread_cancel(consumers[last_idx].thread);
    pthread_join(consumers[last_idx].thread, NULL);
    
    for (int i = last_idx; i < c_cnt - 1; i++) {
        consumers[i] = consumers[i + 1];
    }
    c_cnt--;
    
    printf("Last consumer terminated. Remaining consumers: %d\n", c_cnt);
}

// Показать статус
void show_status(SharedQueue *q) {
    printf("\n--- STATUS ---\n"
           "Queue Size: %d/%d\n"
           "Queue Load: %d/%d\n"
           "Added: %d, Extracted: %d\n"
           "Producers: %d, Consumers: %d\n"
           "--------------\n",
           q->queue_size, q->queue_size,
           q->current_load, q->queue_size,
           q->added_count, q->extracted_count, 
           p_cnt, c_cnt);
}

// Обработчик сигнала
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[Parent] Received signal %d, shutting down...\n", sig);
        keep_running = 0;
        
        // Разблокируем все семафоры
        if (global_queue != NULL) {
            for (int i = 0; i < MAX_THREADS * 2; i++) {
                sem_post(&global_queue->empty);
                sem_post(&global_queue->full);
            }
        }
    }
}

// Функция для чтения команды с таймаутом
int read_command(char *cmd, int size, int timeout_sec) {
    fd_set set;
    struct timeval timeout;
    
    FD_ZERO(&set);
    FD_SET(0, &set);  // STDIN_FILENO
    
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    
    int result = select(1, &set, NULL, NULL, &timeout);
    
    if (result > 0) {
        // Есть данные для чтения
        if (fgets(cmd, size, stdin) != NULL) {
            // Убираем символ новой строки
            size_t len = strlen(cmd);
            if (len > 0 && cmd[len-1] == '\n') {
                cmd[len-1] = '\0';
            }
            return 1;
        }
    }
    return 0; // Нет данных или ошибка
}

int main() {
    reset_keep_running();
    
    SharedQueue queue;
    queue_init(&queue, INITIAL_QUEUE_SIZE);
    global_queue = &queue;
    
    // Установка обработчиков сигналов
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // Перезапускаем системные вызовы
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    printf("  p        - add producer\n");
    printf("  c        - add consumer\n");
    printf("  +        - increase queue size (+1)\n");
    printf("  -        - decrease queue size (-1)\n");
    printf("  kpl      - kill last producer\n");
    printf("  kcl      - kill last consumer\n");
    printf("  s        - show status\n");
    printf("  q        - quit\n");
    printf("Press Ctrl+C for graceful shutdown\n");
    fflush(stdout);

    char cmd[50];
    
    while (keep_running) {
        memset(cmd, 0, sizeof(cmd));
        
        // Ждем команду с таймаутом 1 секунду
        if (read_command(cmd, sizeof(cmd), 1)) {
            if (strcmp(cmd, "p") == 0 && p_cnt < MAX_THREADS) {
                producers[p_cnt].id = p_cnt + 1;
                producers[p_cnt].active = 1;
                if (pthread_create(&producers[p_cnt].thread, NULL, run_producer, &queue) == 0) {
                    p_cnt++;
                    printf("Producer %d started (Thread ID: %lu)\n", p_cnt, producers[p_cnt-1].thread);
                } else {
                    printf("Failed to create producer\n");
                }
            } 
            else if (strcmp(cmd, "c") == 0 && c_cnt < MAX_THREADS) {
                consumers[c_cnt].id = c_cnt + 1;
                consumers[c_cnt].active = 1;
                if (pthread_create(&consumers[c_cnt].thread, NULL, run_consumer, &queue) == 0) {
                    c_cnt++;
                    printf("Consumer %d started (Thread ID: %lu)\n", c_cnt, consumers[c_cnt-1].thread);
                } else {
                    printf("Failed to create consumer\n");
                }
            }
            else if (strcmp(cmd, "+") == 0) {
                queue_resize(&queue, queue.queue_size + 1);
            }
            else if (strcmp(cmd, "-") == 0) {
                if (queue.queue_size > 1) {
                    queue_resize(&queue, queue.queue_size - 1);
                } else {
                    printf("Cannot reduce queue size below 1\n");
                }
            }
            else if (strcmp(cmd, "kpl") == 0) {
                kill_last_producer();
            }
            else if (strcmp(cmd, "kcl") == 0) {
                kill_last_consumer();
            }
            else if (strcmp(cmd, "s") == 0) {
                show_status(&queue);
            }
            else if (strcmp(cmd, "q") == 0) {
                printf("\n[Parent] Shutting down by user request...\n");
                break;
            }
            else if (strlen(cmd) > 0) {
                printf("Unknown command: '%s'\n", cmd);
            }
            
            printf("> ");
            fflush(stdout);
        }
    }

    // Устанавливаем флаг останова
    keep_running = 0;
    
    printf("\n[Parent] Stopping all threads...\n");
    
    // Разблокируем все семафоры
    for (int i = 0; i < MAX_THREADS * 2; i++) {
        sem_post(&queue.empty);
        sem_post(&queue.full);
    }
    
    // Ожидаем завершения производителей
    if (p_cnt > 0) {
        printf("[Parent] Stopping %d producers...\n", p_cnt);
        for (int i = 0; i < p_cnt; i++) {
            if (producers[i].active) {
                pthread_cancel(producers[i].thread);
                pthread_join(producers[i].thread, NULL);
            }
        }
    }
    
    // Ожидаем завершения потребителей
    if (c_cnt > 0) {
        printf("[Parent] Stopping %d consumers...\n", c_cnt);
        for (int i = 0; i < c_cnt; i++) {
            if (consumers[i].active) {
                pthread_cancel(consumers[i].thread);
                pthread_join(consumers[i].thread, NULL);
            }
        }
    }

    // Освобождение ресурсов
    printf("[Parent] Cleaning up resources...\n");
    queue_destroy(&queue);
    
    printf("[Parent] Done. Exiting.\n");
    return 0;
}