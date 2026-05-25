#ifndef LOCKS_H
#define LOCKS_H

#include <sys/types.h>

/* Установка/снятие OFD-блокировки на запись Rec_No.
 * type: F_RDLCK | F_WRLCK | F_UNLCK
 * Возвращает 0 при успехе, -1 при ошибке. */
int lock_record(int fd, int rec_no, short type);

#endif