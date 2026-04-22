#include "common.h"

static volatile sig_atomic_t running = 1;

void consumer_signal_handler (int sig){
    (void)sig;
    running = 0;
}

void consumer (int semid, ring_buffer_t* buf, int id){
    signal(SIGTERM, consumer_signal_handler);
    signal(SIGINT, consumer_signal_handler);
    
    srand(time(NULL) ^ (getpid() << 16)); 

    while (running){
        sem_wait(semid, 1);
        sem_wait(semid, 2);

        message_t* msg = dequeue_message(buf);
        int removed = buf -> removed_count;

        sem_signal(semid, 2);

        sem_signal (semid, 0);

        if(msg){
            unsigned short saved_hash = msg->hash;
            msg->hash = 0;
            unsigned short computed_hash = calculate_hash(msg);
            
            printf("[C%d] - сообщение #%d | size=%d | hash: %s (0x%04X vs 0x%04X)\n", 
                   id, removed, msg->size + 1,
                   (saved_hash == computed_hash) ? "OK" : "FAIL",
                   saved_hash, computed_hash);
            fflush(stdout);
            
            free(msg);
        }

        usleep(300000 + rand() % 400000);
    }
    printf("[C%d] Завершение работы\n", id);
    fflush(stdout);
    exit(0);
}