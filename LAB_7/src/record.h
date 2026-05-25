#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>
#include <stddef.h>

struct record_s {
    char    name[80];      /* Ф.И.О. студента */
    char    address[80];   /* адрес проживания */
    uint8_t semester;      /* семестр */
};

#define REC_SIZE  sizeof(struct record_s)
#define FILENAME  "students.dat"

#endif /* RECORD_H */