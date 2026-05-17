#include "common.h"

/**
 * Основной цикл процесса-потребителя
 * Извлекает сообщения из очереди и проверяет их хеш
 */
void run_consumer(int semid, SharedQueue *q) {
    signal(SIGTERM, handle_sigterm);    // Установка обработчика SIGTERM
    signal(SIGINT, handle_sigterm);     // Установка обработчика SIGINT (Ctrl+C)

    while (keep_running) {
        // Операция P: ждем появления сообщения
        sem_wait(semid, SEM_FULL);
        if (!keep_running) break;       // Проверка после возможного сигнала
        
        // Операция P: захватываем мьютекс очереди
        sem_wait(semid, SEM_MUTEX);

        Message msg = queue_pop(q);     // Критическая секция - извлечение из очереди

        // Операция V: освобождаем мьютекс
        sem_signal(semid, SEM_MUTEX);
        
        // Операция V: сигнализируем о появлении свободного места
        sem_signal(semid, SEM_EMPTY);

        // Проверка целостности сообщения по хешу
        uint16_t check = calculate_DJB2(&msg);
        if (check == msg.hash) 
            printf("[Consumer %d] OK: Hash match\n", getpid());
        else 
            printf("[Consumer %d] ERROR: Hash mismatch!\n", getpid());

        sleep(rand() % 2 + 1);          // Задержка 1-2 секунды
    }
    
    printf("[Consumer %d] Shutting down...\n", getpid());
    exit(0);
}