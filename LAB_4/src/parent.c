#include "common.h"

int main() {
    // Инициализация ресурсов
    int shmid = create_shared_memory(sizeof(SharedQueue));
    SharedQueue *q = (SharedQueue*)attach_shared_memory(shmid);
    memset(q, 0, sizeof(SharedQueue));

    int semid = create_semaphores(3);
    init_semaphores(semid);

    pid_t prods[MAX_PROC], cons[MAX_PROC];
    int p_cnt = 0, c_cnt = 0;

    printf("Commands: p - add prod, c - add cons, s - status, q - quit\n");

    char cmd;
    while (scanf(" %c", &cmd) && cmd != 'q') {
        if (cmd == 'p' && p_cnt < MAX_PROC) {
            pid_t p = fork();
            if (p == 0) run_producer(semid, q);
            else prods[p_cnt++] = p;
        } 
        else if (cmd == 'c' && c_cnt < MAX_PROC) {
            pid_t p = fork();
            if (p == 0) run_consumer(semid, q);
            else cons[c_cnt++] = p;
        } 
        else if (cmd == 's') {
            printf("\n--- STATUS ---\nQueue Load: %d/%d\nAdded: %d, Extracted: %d\nProducers: %d, Consumers: %d\n--------------\n",
                   q->current_load, QUEUE_SIZE, q->added_count, q->extracted_count, p_cnt, c_cnt);
        }
    }

    // Завершение
    printf("Cleaning up...\n");
    for (int i = 0; i < p_cnt; i++) kill(prods[i], SIGTERM);
    for (int i = 0; i < c_cnt; i++) kill(cons[i], SIGTERM);

    while(wait(NULL) > 0); // Ждем завершения всех детей

    destroy_shared_memory(shmid, q);
    semctl(semid, 0, IPC_RMID);
    printf("Done.\n");

    return 0;
}