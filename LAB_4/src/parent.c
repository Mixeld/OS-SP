#include "common.h"

int shmid = -1;
int semid = -1;

ring_buffer_t *shared_buffer = NULL;
process_list_t processes = {0};     //Если править на "динамику" то применяем memset

void cleanup (int signum){
    printf ("\n== Получен сигнал %d. Завершение работы ==\n", signum);

    for (int i = 0; i < processes.prod_count; i++){
        if(processes.producers[i] > 0){
            kill(processes.producers[i], SIGTERM);
        }
    }

    for (int i = 0; i < processes.cons_count; i++){
        if (processes.consumers[i] > 0){
            kill(processes.consumers[i], SIGTERM);
        }
    }

    sleep (1);

    if(semid != -1){
        destroy_semaphores(semid);
    }

    if(shmid != -1 && shared_buffer != NULL){
        destroy_shared_memory(shmid, shared_buffer);
    }

    exit(0);
}

void add_producer(void){

    if (processes.prod_count >= MAX_PRODUCERS){
        printf ("МАКСИМУМ ПРОИЗВОДИТЕЛЕЙ\n");
        return;
    }

    pid_t pid = fork();

    if (pid == 0){
        producer(semid, shared_buffer, processes.prod_count + 1);
        exit(0);
    } else if (pid > 0){
        processes.producers[processes.prod_count++] = pid;
        printf("Создан производителль %d (PID: %d)", processes.prod_count, pid);
    } else {
        perror ("ВИЛКА ЛЕГЛА В ПРОДЮСЕРЕ(С)");
    }
}


void add_consumer(void){
    if(processes.cons_count >= MAX_CONSUMERS){
        printf("МАКСИМУМ ПОТРЕБИТЕЛЕЙ");
        return;
    }

    pid_t pid = fork();

    if (pid == 0){
        consumer (semid, shared_buffer, processes.cons_count+ 1);
        exit (0);
    } else if (pid > 0){
        processes.consumers[processes.cons_count++] = pid;
        printf("Создан потребитель %d (PID: %d)", processes.cons_count, pid);
    } else {
        perror ("ВИЛКА ЛЕГЛА В ПОТРЕБИТЕЛЕ(С)");
    }
}


void kill_producer (void){

    if(processes.prod_count == 1&& processes.cons_count > 0){
        printf("Нельзя убивать послднего производителя, потребители иначе лягут");
        return;
    }

    if(processes.prod_count == 0){
        printf("НЕТ ПРОИЗВОДИТЕЛЕЙ");
        return;
    }

    int   idx = processes.prod_count - 1;
    pid_t pid = processes.producers[idx];

    if(kill(pid,SIGTERM) == 0){
        printf("Убит производитель %d (PID: %d)", idx + 1, pid);
        processes.prod_count--;
    } else {
        perror ("ВИЛКА ЛЕГЛЕ В ПРОДЮСЕРЕ (У)");
    }
}

void kill_consumer (void){

        if(processes.cons_count == 1&& processes.prod_count > 0){
        printf("Нельзя убивать послднего Потребителя, очередь переполнится");
        return;
    }

    if(processes.cons_count == 0){
        printf("МАКСИМУМ ПОТРЕБИТЕЛЕЙ");
        return;
    }

    int   idx = processes.cons_count - 1;
    pid_t pid = processes.consumers[idx];

    if(kill(pid,SIGTERM) == 0){
        printf("Убит поребитель %d (PID: %d)", idx + 1, pid);
        processes.cons_count--;
    } else {
        perror("ВИЛКА ЛЕГЛА В ПОТРЕБИТЕЛЕ (У)");
    }
}


void show_status (void){
    printf("\n=== Состояние ===\n");
    printf("Производителей: %d\n", processes.prod_count);
    printf("Потребителей: %d\n", processes.cons_count);

    if(shared_buffer != NULL){
        int used = get_queue_used(shared_buffer);
        int free = get_queue_free(shared_buffer);
        printf("Очередь: занято %d, свободно %d, всего %d\n",used, free, QUEUE_SIZE);
        printf("Всего добавлено: %d\n", shared_buffer -> added_count);
        printf("Всего извлечено: %d\n", shared_buffer->removed_count);
    }
    printf("=================");
}

int main (void){

    shmid = create_shared_memory(SHM_KEY, sizeof(ring_buffer_t));
    shared_buffer = (ring_buffer_t*) attach_shared_memory(shmid);

    init_ring_buffer(shared_buffer, QUEUE_SIZE);

    semid = create_semaphores(SEM_KEY, 3);
    init_semaphores(semid, QUEUE_SIZE);

    signal (SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGQUIT, cleanup);


    char cmd;
    while(1){
        cmd = getchar();
        getchar();

        switch (cmd) {
            case 'p': add_producer (); break;
            case 'c': add_consumer();  break;
            case 'k': kill_producer(); break;
            case 'l': kill_consumer(); break;
            case 's': show_status();   break;
            case 'q': cleanup(0);      break;
            default: printf ("Неизвестная команда\n");
        }
    }

    return 0;

}