#include "common.h"

Queue g_queue;

Producer g_producers[MAX_PRODUCERS];
Consumer g_consumers[MAX_CONSUMERS];

int g_producer_count = 0;
int g_consumer_count = 0;
int g_running = 1;

sem_t g_sem_empty;
sem_t g_sem_full;

pthread_mutex_t g_mutex;
pthread_mutex_t g_print_mutex;

pthread_t g_prod_threads[MAX_PRODUCERS];
pthread_t g_cons_threads[MAX_CONSUMERS];


static void init_producer_array(void){
    for (int i = 0; i < MAX_PRODUCERS; i ++){
        g_producers[i].id = i + 1;
        g_producers[i].produced = 0;
        g_producers[i].active = 0;
    }
}

static void init_consumer_array (void){
    for (int i = 0; i < MAX_CONSUMERS; i++) {
        g_consumers[i].id = i + 1;
        g_consumers[i].active = 0;
        g_consumers[i].consumed = 0;
    }
}

int ipc_init (void){
    if (queue_init(INITIAL_QUEUE_SIZE) != 0){
        return -1;
    }
    
    if(sem_init(&g_sem_empty,0,0) != 0){
        queue_destroy();
        return -1;
    }

    if (sem_init(&g_sem_full, 0, 0) != 0){
        sem_destroy(&g_sem_empty);
        queue_destroy();
        return -1;
    }

    if (pthread_mutex_init(&g_mutex, 0) != 0){
        sem_destroy(&g_sem_empty);
        sem_destroy(&g_sem_full);
        queue_destroy();
        return -1;
    }

    g_producer_count = 0;
    g_consumer_count = 0;
    g_running = 1;

    init_producer_array();
    init_consumer_array();

    return 0;
}


void ipc_destroy (void){
    pthread_mutex_destroy (&g_print_mutex);
    pthread_mutex_destroy (&g_mutex);

    sem_destroy(&g_sem_empty);
    sem_destroy(&g_sem_full);

    queue_destroy();
}