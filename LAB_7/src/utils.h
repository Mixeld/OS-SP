#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* Чтение строки с stdin, обрезает '\n'. Возвращает 1 если ок, 0 если EOF. */
int read_line(char *buf, size_t sz);

#endif