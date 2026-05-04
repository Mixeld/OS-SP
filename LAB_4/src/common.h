#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define QUEUE_SIZE 10
#define MAX_PROC 10

// 1. Сначала сообщение
typedef struct {
    uint8_t type;
    uint16_t hash;
    uint8_t size;
    unsigned char data[260];
} Message;

// 2. Потом очередь
typedef struct {
    Message buffer[QUEUE_SIZE];
    int head;
    int tail;
    int added_count;
    int extracted_count;
    int current_load;
} SharedQueue;

// Индексы семафоров
#define SEM_EMPTY 0
#define SEM_FULL 1
#define SEM_MUTEX 2

// Глобальные переменные сигналов (объявление)
extern volatile sig_atomic_t keep_running;
void handle_sig(int sig);

// Прототипы функций IPC
int create_shared_memory(size_t size);
void* attach_shared_memory(int shmid);
void destroy_shared_memory(int shmid, void* shmaddr);
int create_semaphores(int nsems);
void init_semaphores(int semid);
void sem_wait(int semid, int sem_num);
void sem_signal(int semid, int sem_num);

// Прототипы логики
uint16_t calculate_DJB2(Message *msg);
void create_random_message(Message *msg);
void queue_push(SharedQueue *q, Message *msg);
Message queue_pop(SharedQueue *q);

// Прототипы процессов
void run_producer(int semid, SharedQueue *q);
void run_consumer(int semid, SharedQueue *q);

#endif