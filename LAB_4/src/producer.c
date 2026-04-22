#include "common.h"

static volatile sig_atomic_t running = 1;

void producer_signal_handler(int sig){
    (void)sig;
    running = 0;
}

void producer (int semid, ring_buffer_t* buf, int id){  
    signal(SIGTERM, producer_signal_handler);
    signal(SIGINT, producer_signal_handler);    

    srand(time(NULL) ^ (getpid() << 16));

    while (running){
        message_t* msg = (message_t*)malloc(sizeof(message_t));
        if(!msg){
            perror("malloc fail (P)");
            break;
        }

        msg->type = rand() % 256;
        msg->size = rand() % 256;

        for (int i = 0; i <= msg->size; i++){
            msg->data[i] = rand() % 256;
        }

        msg->hash = 0;
        msg->hash = calculate_hash(msg);

        sem_wait(semid, 0);
        sem_wait(semid, 2);

        enqueue_message(buf, msg);  
        int added = buf->added_count;

        sem_signal(semid, 2);
        sem_signal(semid, 1);

        printf("[P%d] + сообщение #%d | size=%d | hash=0x%04X\n", 
               id, added, msg->size + 1, msg->hash);
        fflush(stdout);

        usleep(300000 + rand() % 400000);
    }

    printf("[P%d] Завершение работы\n", id);
    fflush(stdout);
    exit(0);
}