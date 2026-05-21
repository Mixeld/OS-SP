#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdatomic.h>

#define INITIAL_QUEUE_SIZE 10
#define MAX_THREADS 10

// Сообщение
typedef struct {
    uint8_t type;
    uint16_t hash;
    uint8_t size;
    unsigned char data[260];
} Message;

// Очередь с условными переменными
typedef struct {
    Message *buffer;
    int head;
    int tail;
    int added_count;
    int extracted_count;
    int current_load;
    int queue_size;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;  
    pthread_cond_t cond_not_full;   
} SharedQueue;

// Структура для хранения информации о потоках
typedef struct {
    pthread_t thread;
    int id;
    int active;
    long produced_count;
    long consumed_count;
} ThreadInfo;

// Глобальные переменные
extern volatile sig_atomic_t keep_running;
extern ThreadInfo producers[MAX_THREADS];
extern ThreadInfo consumers[MAX_THREADS];
extern int p_cnt;
extern int c_cnt;
extern pthread_mutex_t global_mutex;

// Функции сигналов
void handle_sigterm(int sig);
void handle_sigint(int sig);
void reset_keep_running(void);

// Функции очереди
void queue_init(SharedQueue *q, int size);
void queue_destroy(SharedQueue *q);
int queue_push(SharedQueue *q, Message *msg);
Message queue_pop(SharedQueue *q);
int queue_resize(SharedQueue *q, int new_size);

// Функции сообщений
uint16_t calculate_DJB2(Message *msg);
void create_random_message(Message *msg);
uint16_t message_calc_hash(const Message *msg);
int message_check_hash(const Message *msg);
void message_generate(Message *msg);

// Функции потоков
void* run_producer(void *arg);
void* run_consumer(void *arg);

// Управление потоками
void kill_last_producer(void);
void kill_last_consumer(void);
void show_status(SharedQueue *q);

// Функции очистки памяти
void cleanup_all(SharedQueue *q);
void cancel_all_threads(void);
void join_all_threads(void);

#endif