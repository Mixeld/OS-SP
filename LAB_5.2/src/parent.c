#include "common.h"
#include <string.h>
#include <sys/select.h>

SharedQueue *global_queue = NULL;

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
    
    printf("Killing last producer (Thread ID: %lu, produced: %ld)\n", 
           producers[last_idx].thread, producers[last_idx].produced_count);
    producers[last_idx].active = 0;
    pthread_cancel(producers[last_idx].thread);
    pthread_join(producers[last_idx].thread, NULL);
    
    for (int i = last_idx; i < p_cnt - 1; i++) {
        producers[i] = producers[i + 1];
    }
    p_cnt--;
    
    printf("Last producer terminated. Remaining producers: %d\n", p_cnt);
}

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
    
    printf("Killing last consumer (Thread ID: %lu, consumed: %ld)\n", 
           consumers[last_idx].thread, consumers[last_idx].consumed_count);
    consumers[last_idx].active = 0;
    pthread_cancel(consumers[last_idx].thread);
    pthread_join(consumers[last_idx].thread, NULL);
    
    for (int i = last_idx; i < c_cnt - 1; i++) {
        consumers[i] = consumers[i + 1];
    }
    c_cnt--;
    
    printf("Last consumer terminated. Remaining consumers: %d\n", c_cnt);
}

void show_status(SharedQueue *q) {
    int queue_size, current_load, added_count, extracted_count;
    
    pthread_mutex_lock(&q->mutex);
    queue_size = q->queue_size;
    current_load = q->current_load;
    added_count = q->added_count;
    extracted_count = q->extracted_count;
    pthread_mutex_unlock(&q->mutex); 
    
    printf("\n--- STATUS ---\n");
    printf("Queue Size: %d/%d\n", queue_size, queue_size);
    printf("Queue Load: %d/%d\n", current_load, queue_size);
    printf("Total Added: %d, Total Extracted: %d\n", added_count, extracted_count);
    printf("Active Producers: %d, Active Consumers: %d\n", p_cnt, c_cnt);
    
    printf("\nProducer statistics:\n");
    for (int i = 0; i < p_cnt; i++) {
        if (producers[i].active) {
            printf("  Producer %d: produced %ld messages\n", 
                   i, producers[i].produced_count);
        }
    }
    
    printf("\nConsumer statistics:\n");
    for (int i = 0; i < c_cnt; i++) {
        if (consumers[i].active) {
            printf("  Consumer %d: consumed %ld messages\n", 
                   i, consumers[i].consumed_count);
        }
    }
    printf("--------------\n");
    
    fflush(stdout);
}

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[Parent] Received signal %d, shutting down...\n", sig);
        keep_running = 0;
        
        if (global_queue != NULL) {
            pthread_mutex_lock(&global_queue->mutex);
            pthread_cond_broadcast(&global_queue->cond_not_empty);
            pthread_cond_broadcast(&global_queue->cond_not_full);
            pthread_mutex_unlock(&global_queue->mutex);
        }
    }
}

int read_command(char *cmd, int size, int timeout_sec) {
    fd_set set;
    struct timeval timeout;
    
    FD_ZERO(&set);
    FD_SET(0, &set);
    
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    
    int result = select(1, &set, NULL, NULL, &timeout);
    
    if (result > 0) {
        if (fgets(cmd, size, stdin) != NULL) {
            size_t len = strlen(cmd);
            if (len > 0 && cmd[len-1] == '\n') {
                cmd[len-1] = '\0';
            }
            return 1;
        }
    }
    return 0;
}

int main() {
    reset_keep_running();
    
    SharedQueue queue;
    queue_init(&queue, INITIAL_QUEUE_SIZE);
    global_queue = &queue;
    
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
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
    printf("> ");
    fflush(stdout);
    
    char cmd[50];
    
    while (1) {
        pthread_mutex_lock(&global_mutex);
        int running = keep_running;
        pthread_mutex_unlock(&global_mutex);
        
        if (!running) break;
        
        memset(cmd, 0, sizeof(cmd));
        
        if (read_command(cmd, sizeof(cmd), 1)) {
            if (strcmp(cmd, "p") == 0 && p_cnt < MAX_THREADS) {
                producers[p_cnt].id = p_cnt + 1;
                producers[p_cnt].active = 1;
                producers[p_cnt].produced_count = 0;
                if (pthread_create(&producers[p_cnt].thread, NULL, 
                                   run_producer, &queue) == 0) {
                    p_cnt++;
                    printf("Producer %d started (Thread ID: %lu)\n", 
                           p_cnt, producers[p_cnt-1].thread);
                } else {
                    printf("Failed to create producer\n");
                }
            } 
            else if (strcmp(cmd, "c") == 0 && c_cnt < MAX_THREADS) {
                consumers[c_cnt].id = c_cnt + 1;
                consumers[c_cnt].active = 1;
                consumers[c_cnt].consumed_count = 0;
                if (pthread_create(&consumers[c_cnt].thread, NULL, 
                                   run_consumer, &queue) == 0) {
                    c_cnt++;
                    printf("Consumer %d started (Thread ID: %lu)\n", 
                           c_cnt, consumers[c_cnt-1].thread);
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
    
    printf("\n[Parent] Starting graceful shutdown...\n");
    cleanup_all(&queue);
    
    printf("[Parent] Done. Exiting.\n");
    return 0;
}