#include "common.h"

// Определение глобального флага (только здесь)
volatile sig_atomic_t keep_running = 1;

void reset_keep_running(void){
    keep_running = 1;
}

/**
 * Обработчик сигнала SIGTERM
 * Устанавливает флаг keep_running в 0 для graceful shutdown
 */
void handle_sigterm(int sig) {
    (void) sig;
    keep_running = 0;
}

/**
 * Обработчик сигнала SIGINT (Ctrl+C)
 * Устанавливает флаг keep_running в 0 для graceful shutdown
 * Выводит сообщение о завершении
 */
void handle_sigint(int sig) {
    (void) sig;
    printf("\n[Parent] Received Ctrl+C, shutting down...\n");
    keep_running = 0;
}

/**
 * Создание сегмента разделяемой памяти
 */
int create_shared_memory(size_t size) {
    int shmid = shmget(SHM_KEY, size, IPC_CREAT | 0666);
    if (shmid == -1) { 
        perror("shmget failed"); 
        exit(1); 
    }
    return shmid;
}

/**
 * Подключение разделяемой памяти к адресному пространству процесса
 */
void* attach_shared_memory(int shmid) {
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) { 
        perror("shmat failed"); 
        exit(1); 
    }
    return addr;
}

/**
 * Отключение и удаление разделяемой памяти
 */
void destroy_shared_memory(int shmid, void* shmaddr) {
    shmdt(shmaddr);           // Отключение
    shmctl(shmid, IPC_RMID, NULL); // Удаление
}

/**
 * Создание набора семафоров
 */
int create_semaphores(int nsems) {
    int semid = semget(SEM_KEY, nsems, IPC_CREAT | 0666);
    if (semid == -1) { 
        perror("semget failed"); 
        exit(1); 
    }
    return semid;
}

/**
 * Инициализация семафоров:
 * SEM_EMPTY = QUEUE_SIZE (свободных мест)
 * SEM_FULL = 0 (занятых мест)
 * SEM_MUTEX = 1 (мьютекс разблокирован)
 */
void init_semaphores(int semid) {
    semctl(semid, SEM_EMPTY, SETVAL, QUEUE_SIZE);
    semctl(semid, SEM_FULL, SETVAL, 0);
    semctl(semid, SEM_MUTEX, SETVAL, 1);
}

/**
 * P-операция (wait) над семафором
 * Блокирует процесс, если значение семафора = 0
 */
void sem_wait(int semid, int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    while (semop(semid, &op, 1) == -1) {
        if (errno != EINTR) { 
            perror("sem_wait error"); 
            exit(1); 
        }
        if (!keep_running) return; // Выход при получении сигнала
    }
}

/**
 * V-операция (signal) над семафором
 * Увеличивает значение семафора, разблокируя ожидающие процессы
 */
void sem_signal(int semid, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    while (semop(semid, &op, 1) == -1) {
        if (errno != EINTR) { 
            perror("sem_signal error"); 
            exit(1); 
        }
    }
}

/**
 * Вычисление хеша сообщения по алгоритму DJB2
 * Алгоритм: hash = hash * 33 + byte
 * 16-битный хеш
 */
uint16_t calculate_DJB2(Message *msg) {
    unsigned long hash = 5381;
    
    // Хешируем поля сообщения
    hash = ((hash << 5) + hash) + msg->type;
    hash = ((hash << 5) + hash) + 0; // старший байт hash (всегда 0)
    hash = ((hash << 5) + hash) + 0; // младший байт hash (всегда 0)
    hash = ((hash << 5) + hash) + msg->size;
    
    // Хешируем данные (size+1 байт для учета нуль-терминатора)
    for (int i = 0; i < (msg->size + 1); i++) {
        hash = ((hash << 5) + hash) + msg->data[i];
    }
    
    return (uint16_t)hash;
}

/**
 * Генерация случайного сообщения с корректным хешем
 * Данные выравниваются до размера, кратного 4 байтам
 */
void create_random_message(Message *msg) {
    msg->type = rand() % 256;
    msg->size = rand() % 256;
    
    // Выравнивание данных до границы 4 байт
    int real_len = msg->size + 1;              // +1 для нуль-терминатора
    int aligned_len = ((msg->size + 4) / 4) * 4; // Округление вверх до 4
    
    for (int i = 0; i < aligned_len; i++) {
        msg->data[i] = (i < real_len) ? (rand() % 256) : 0;
    }
    
    msg->hash = calculate_DJB2(msg); // Вычисляем хеш после заполнения данных
}

/**
 * Добавление сообщения в кольцевую очередь (производитель)
 */
void queue_push(SharedQueue *q, Message *msg) {
    q->buffer[q->tail] = *msg;                  // Копируем сообщение
    q->tail = (q->tail + 1) % QUEUE_SIZE;       // Сдвигаем указатель записи
    q->added_count++;                           // Увеличиваем счетчик добавленных
    q->current_load++;                          // Увеличиваем текущую загрузку
    
    printf("[Producer %d] Added message #%d. In queue: %d\n", 
           getpid(), q->added_count, q->current_load);
}

/**
 * Извлечение сообщения из кольцевой очереди (потребитель)
 */
Message queue_pop(SharedQueue *q) {
    Message msg = q->buffer[q->head];           // Копируем сообщение
    q->head = (q->head + 1) % QUEUE_SIZE;       // Сдвигаем указатель чтения
    q->extracted_count++;                       // Увеличиваем счетчик извлеченных
    q->current_load--;                          // Уменьшаем текущую загрузку
    
    printf("[Consumer %d] Popped message #%d. In queue: %d\n", 
           getpid(), q->extracted_count, q->current_load);
    
    return msg;
}