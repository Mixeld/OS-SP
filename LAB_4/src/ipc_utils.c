#include "common.h"

// ОПРЕДЕЛЕНИЕ переменной и функции сигнала (только в одном файле!)
volatile sig_atomic_t keep_running = 1;
void handle_sig(int sig) {
    keep_running = 0;
}

int create_shared_memory(size_t size) {
    int shmid = shmget(SHM_KEY, size, IPC_CREAT | 0666);
    if (shmid == -1) { perror("shmget failed"); exit(1); }
    return shmid;
}

void* attach_shared_memory(int shmid) {
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) { perror("shmat failed"); exit(1); }
    return addr;
}

void destroy_shared_memory(int shmid, void* shmaddr) {
    shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);
}

int create_semaphores(int nsems) {
    int semid = semget(SEM_KEY, nsems, IPC_CREAT | 0666);
    if (semid == -1) { perror("semget failed"); exit(1); }
    return semid;
}

void init_semaphores(int semid) {
    semctl(semid, SEM_EMPTY, SETVAL, QUEUE_SIZE);
    semctl(semid, SEM_FULL, SETVAL, 0);
    semctl(semid, SEM_MUTEX, SETVAL, 1);
}

void sem_wait(int semid, int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    while (semop(semid, &op, 1) == -1) {
        if (errno != EINTR) { perror("sem_wait error"); exit(1); }
        if (!keep_running) return; // Выход если получили сигнал
    }
}

void sem_signal(int semid, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    while (semop(semid, &op, 1) == -1) {
        if (errno != EINTR) { perror("sem_signal error"); exit(1); }
    }
}

uint16_t calculate_DJB2(Message *msg) {
    unsigned long hash = 5381;
    hash = ((hash << 5) + hash) + msg->type;
    hash = ((hash << 5) + hash) + 0; // hash field byte 1
    hash = ((hash << 5) + hash) + 0; // hash field byte 2
    hash = ((hash << 5) + hash) + msg->size;
    for (int i = 0; i < (msg->size + 1); i++) {
        hash = ((hash << 5) + hash) + msg->data[i];
    }
    return (uint16_t)hash;
}

void create_random_message(Message *msg) {
    msg->type = rand() % 256;
    msg->size = rand() % 256;
    int real_len = msg->size + 1;
    int aligned_len = ((msg->size + 4) / 4) * 4;
    for (int i = 0; i < aligned_len; i++) {
        msg->data[i] = (i < real_len) ? (rand() % 256) : 0;
    }
    msg->hash = calculate_DJB2(msg);
}

void queue_push(SharedQueue *q, Message *msg) {
    q->buffer[q->tail] = *msg;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    q->added_count++;
    q->current_load++;
    printf("[Producer %d] Added message #%d. In queue: %d\n", getpid(), q->added_count, q->current_load);
}

Message queue_pop(SharedQueue *q) {
    Message msg = q->buffer[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->extracted_count++;
    q->current_load--;
    printf("[Consumer %d] Popped message #%d. In queue: %d\n", getpid(), q->extracted_count, q->current_load);
    return msg;
}