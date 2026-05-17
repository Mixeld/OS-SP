#include "common.h"

/**
 * Основной цикл процесса-производителя
 * Генерирует случайные сообщения и помещает их в очередь
 */
void run_producer(int semid, SharedQueue *q) {
    srand(getpid());                    // Инициализация генератора случайных чисел
    signal(SIGTERM, handle_sigterm);    // Установка обработчика SIGTERM
    signal(SIGINT, handle_sigterm);     // Установка обработчика SIGINT (Ctrl+C)

    while (keep_running) {
        Message msg;
        create_random_message(&msg);    // Создаем случайное сообщение

        // Операция P: ждем свободное место в очереди
        sem_wait(semid, SEM_EMPTY);
        if (!keep_running) break;       // Проверка после возможного сигнала
        
        // Операция P: захватываем мьютекс очереди
        sem_wait(semid, SEM_MUTEX);

        queue_push(q, &msg);            // Критическая секция - добавление в очередь

        // Операция V: освобождаем мьютекс
        sem_signal(semid, SEM_MUTEX);
        
        // Операция V: сигнализируем о появлении нового сообщения
        sem_signal(semid, SEM_FULL);

        sleep(rand() % 2 + 1);          // Задержка 1-2 секунды
    }
    
    printf("[Producer %d] Shutting down...\n", getpid());
    exit(0);
}