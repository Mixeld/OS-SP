// common.h
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
#define MAX_PRODUCERS 10
#define MAX_CONSUMERS 10

typedef struct {
    void* buffer[QUEUE_SIZE];
    int head;
    int tail;
    int added_count;
    int removed_count;
} ring_buffer_t;

typedef struct {
    pid_t producers[MAX_PRODUCERS];
    pid_t consumers[MAX_CONSUMERS];
    int prod_count;
    int cons_count;
} process_list_t;

typedef struct {
    unsigned char type;
    unsigned short hash;
    unsigned char size;
    unsigned char data[256];
} message_t;

// Функции для работы с разделяемой памятью
int create_shared_memory(key_t key, size_t size);
void* attach_shared_memory(int shmid);
void destroy_shared_memory(int shmid, void* shmaddr);

// Функции для работы с семафорами
int create_semaphores(key_t key, int nsems);
void init_semaphores(int semid, int queue_size);
void destroy_semaphores(int semid);
void sem_wait (int semid, int sem_num);
void sem_signal (int semid, int sem_num);

// Функции для работы с очередью
void init_ring_buffer(ring_buffer_t* buf, int size);
int get_queue_used(ring_buffer_t* buf);
int get_queue_free(ring_buffer_t* buf);
int enqueue_message(ring_buffer_t* buf, message_t* msg);
message_t* dequeue_message(ring_buffer_t* buf);

// Функции производителя и потребителя
void producer(int semid, ring_buffer_t* buf, int id);
void consumer(int semid, ring_buffer_t* buf, int id);

//Функция хэширования
unsigned short calculate_hash (message_t* msg);

