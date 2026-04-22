#include "common.h"

// РЕАЛИЗАЦИЯ РАЗДЕЛЯЕМОЙ ПАМЯТИ

int create_shared_memory(key_t key, size_t size){
    int shmid = shmget(key,size,IPC_CREAT | 0666);  //создание сегмента раздельной памяти
    if (shmid == -1){
        perror("shmget failed");
        exit(1);
    }
    return shmid;
}

void* attach_shared_memory(int shmid){
    void* addr = shmat(shmid, NULL, 0);
    if(addr == (void*)-1){
        perror("shmat failed");
        exit(1);
    }
    return addr;
}

void destroy_shared_memory(int shmid, void* shmaddr){
    shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);          //IPC_RMID - Удалить из системы идентификатор semid
}

// СЕМАФОРЫ 

int create_semaphores (key_t key, int nsems){
    int semid = semget(key, nsems, IPC_CREAT | 0666);
    if (semid == -1){
        perror("semget failed");
        exit(1);
    }
    return semid;
}

void init_semaphores(int semid, int queue_size) {

    //SETVAL - Установить значение семафора равным arg.val. 

    semctl (semid, 0, SETVAL, queue_size);  //пусто
    semctl (semid, 1, SETVAL, 0);           //полный
    semctl (semid, 2, SETVAL, 1);           //mutex
}


void destroy_semaphores (int semid){
    semctl (semid, 0, IPC_RMID);        // IPC_RMID - Удалить из системы идентификатор semid
}

void sem_wait (int semid, int sem_num){
    struct sembuf op = {sem_num, -1, 0};
    if(semop(semid, &op, 1) == -1){
        perror("sem_wait failed");
        exit(1);
    }
}

void sem_signal (int semid, int sem_num){
    struct sembuf op = {sem_num, +1, 0};
    if (semop(semid, &op, 1) == -1){
        perror ("sem_signai failed");
        exit(1);
    }
}
