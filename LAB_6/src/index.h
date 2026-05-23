
#ifndef INDEX_H
#define INDEX_H

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>

struct index_s {
    double   time_mark; // временная метка (ключ сортировки)
    uint64_t recno;     // номер записи в БД
};

struct index_hdr_s {
    uint64_t records;   // количество записей
    struct index_s idx[]; // гибкий массив
};

struct cli_args {
    size_t memsize;
    int blocks;
    int threads; 
    const char *filename;
};

struct block_info
{
    int state;          //0 = free, 1 = busy, 2 = done 
    size_t offset;
    size_t size;
};

struct chunk_info {
    off_t file_offset;
    size_t records;
};


struct sort_context {
    int fd;
    size_t memsize;
    int blocks;
    int threads;

    long pagesize;

    struct index_s *buf;
    size_t chunk_records;

    struct block_info *block_map;
    int active_blocks;

    pthread_mutex_t map_mutex;
    pthread_barrier_t control_barrier;
    pthread_barrier_t workers_barrier;

    int phase;
    int done;
    int merge_task_count;
};   

struct thread_args {
    int thread_id;
    struct sort_context *ctx;
};


#endif 