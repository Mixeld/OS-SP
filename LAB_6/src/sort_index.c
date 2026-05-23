#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "index.h"

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <memsize> <blocks> <threads> <filename>\n", prog);
    fprintf(stderr, "  memsize  - working buffer size in bytes, multiple of page size\n");
    fprintf(stderr, "  blocks   - number of blocks, power of two\n");
    fprintf(stderr, "  threads  - number of threads\n");
    fprintf(stderr, "  filename - index file name\n");
}

static int is_power_of_two(int x) {
    return x > 0 && (x & (x - 1)) == 0;
}

static int validate_cli(const struct cli_args *args) {
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize < 0) {
        perror("sysconf");
        return -1;
    }

    if (args->memsize == 0 || (args->memsize % (size_t)pagesize) != 0) {
        fprintf(stderr, "memsize must be > 0 and multiple of page size (%ld)\n", pagesize);
        return -1;
    }

    if (!is_power_of_two(args->blocks)) {
        fprintf(stderr, "blocks must be a power of two\n");
        return -1;
    }

    if (args->threads <= 0) {
        fprintf(stderr, "threads must be > 0\n");
        return -1;
    }

    if (args->blocks < 4 * args->threads) {
        fprintf(stderr, "blocks must be at least 4 * threads\n");
        return -1;
    }

    return 0;
}

static int parse_size (const char *s, size_t *out){
    char *end = NULL;
    errno = 0;

    unsigned long long val = strtoull (s, &end, 10);

    if (errno != 0 || end == s || *end != '\0'){
        return -1;
    }

    *out = (size_t) val;
    return 0; 
}

static int parse_int (const char *s, int *out) {
    char *end = NULL;
    errno = 0;
    long val = strtol(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0'){
        return -1;
    }

    if (val < INT_MIN || val > INT_MAX){
        return -1;
    }

    *out = (int)val;

    return 0;
}

static int parse_cli (int argc, char *argv[], struct cli_args *args){
    if (argc != 5) {
        usage(argv[0]);
        return -1;
    }

    if (parse_size(argv[1], &args->memsize) != 0) {
        fprintf(stderr, "Invalid memsize: %s\n", argv[1]);
        return -1;
    }

    if (parse_int(argv[2], &args->blocks) != 0) {
        fprintf(stderr, "Invalid blocks: %s\n", argv[2]);
        return -1;
    }

    if (parse_int(argv[3], &args->threads) != 0) {
        fprintf(stderr, "Invalid threads: %s\n", argv[3]);
        return -1;
    }

    args->filename = argv[4];

    return 0;
}

static int cmp_index(const void *a, const void *b) {
    const struct index_s *ia = (const struct index_s *)a;
    const struct index_s *ib = (const struct index_s *)b;

    if (ia->time_mark < ib->time_mark) return -1;
    if (ia->time_mark > ib->time_mark) return 1;
    return 0;
}

static void merge_two_runs (struct index_s *arr, size_t left_count, size_t right_count){
    size_t total = left_count + right_count;
    struct index_s *tmp = malloc(total * sizeof(struct index_s));

    if (!tmp){
        perror ("malloc");
        exit (EXIT_FAILURE);
    }
    size_t i =0;
    size_t j = 0;
    size_t k =0;

    while (i < left_count && j < right_count){
        if (arr[i].time_mark <= arr[left_count + j].time_mark){
            tmp[k++] = arr[i++];
        } else {
            tmp[k++] = arr[left_count + j++];
        }
    }

    while (i < left_count){
        tmp[k++] = arr[i++];
    }

    while (j < right_count){
        tmp[k++] = arr[left_count + j++];
    }

    memcpy(arr, tmp, total * sizeof(struct index_s));
    free (tmp);
}

static int get_free_block_for_sort(struct sort_context *ctx) {
    int result = -1;

    pthread_mutex_lock(&ctx->map_mutex);
    for (int i = 0; i < ctx->active_blocks; ++i) {
        if (ctx->block_map[i].state == 0) {
            ctx->block_map[i].state = 1;
            result = i;
            break;
        }
    }
    pthread_mutex_unlock(&ctx->map_mutex);

    return result;
}

static int get_free_pair_for_merge(struct sort_context *ctx, int pair_count) {
    int result = -1;

    pthread_mutex_lock(&ctx->map_mutex);
    for (int i = 0; i < pair_count; i++) {
        if (ctx->block_map[i].state == 0) {
            ctx->block_map[i].state = 1;  
            result = i;                   
            break;
        }
    }
    pthread_mutex_unlock(&ctx->map_mutex);

    return result;
}

static void *worker_thread(void *arg) {
    struct thread_args *ta = (struct thread_args *)arg;
    struct sort_context *ctx = ta->ctx;
    int tid = ta->thread_id;

    while (1) {
        pthread_barrier_wait(&ctx->control_barrier);

        if (ctx->phase == 2 || ctx->done) {
            break;
        }

        if (ctx->phase == 1) {
            /* ФАЗА 1: локальная сортировка блоков */
            while (1) {
                int blk = get_free_block_for_sort(ctx);
                if (blk < 0) {
                    break;
                }

                qsort(ctx->buf + ctx->block_map[blk].offset,
                      ctx->block_map[blk].size,
                      sizeof(struct index_s),
                      cmp_index);

                pthread_mutex_lock(&ctx->map_mutex);
                ctx->block_map[blk].state = 2;
                pthread_mutex_unlock(&ctx->map_mutex);
            }

            pthread_barrier_wait(&ctx->workers_barrier);

            /* ФАЗА 2: merge-раунды */
            while (1) {
                int active = ctx->active_blocks;
                if (active <= 1) {
                    break;
                }

                int pair_count = active / 2;

                if (pair_count >= ctx->threads) {
                    /* ранние раунды: каждому потоку своя пара по tid */
                    int pair_index = tid;
                    int left_index = pair_index * 2;
                    int right_index = left_index + 1;

                    if (pair_index < pair_count && right_index < active) {
                        size_t left_off = ctx->block_map[left_index].offset;
                        size_t left_sz  = ctx->block_map[left_index].size;
                        size_t right_sz = ctx->block_map[right_index].size;

                        merge_two_runs(ctx->buf + left_off, left_sz, right_sz);
                    }

                    pthread_barrier_wait(&ctx->workers_barrier);

                    if (tid == 0) {
                        int new_active = 0;

                        for (int i = 0; i < active; i += 2) {
                            if (i + 1 < active) {
                                ctx->block_map[new_active].offset =
                                    ctx->block_map[i].offset;
                                ctx->block_map[new_active].size =
                                    ctx->block_map[i].size + ctx->block_map[i + 1].size;
                                ctx->block_map[new_active].state = 0;
                            } else {
                                ctx->block_map[new_active] = ctx->block_map[i];
                            }

                            new_active++;
                        }

                        ctx->active_blocks = new_active;
                    }

                    pthread_barrier_wait(&ctx->workers_barrier);
                } else {
                    /* поздние раунды: пар меньше, чем потоков */
                    if (tid == 0) {
                        for (int i = 0; i < pair_count; ++i) {
                            ctx->block_map[i].state = 0;
                        }
                        ctx->merge_task_count = pair_count;
                    }

                    pthread_barrier_wait(&ctx->workers_barrier);

                    while (1) {
                        int pair_index = get_free_pair_for_merge(ctx, pair_count);
                        if (pair_index < 0) {
                            break;
                        }

                        int left_index = pair_index * 2;
                        int right_index = left_index + 1;

                        if (right_index < active) {
                            size_t left_off = ctx->block_map[left_index].offset;
                            size_t left_sz  = ctx->block_map[left_index].size;
                            size_t right_sz = ctx->block_map[right_index].size;

                            merge_two_runs(ctx->buf + left_off, left_sz, right_sz);

                            pthread_mutex_lock(&ctx->map_mutex);
                            ctx->block_map[pair_index].state = 2;
                            pthread_mutex_unlock(&ctx->map_mutex);
                        }
                    }

                    pthread_barrier_wait(&ctx->workers_barrier);

                    if (tid == 0) {
                        int new_active = 0;

                        for (int i = 0; i < active; i += 2) {
                            if (i + 1 < active) {
                                ctx->block_map[new_active].offset =
                                    ctx->block_map[i].offset;
                                ctx->block_map[new_active].size =
                                    ctx->block_map[i].size + ctx->block_map[i + 1].size;
                                ctx->block_map[new_active].state = 0;
                            } else {
                                ctx->block_map[new_active] = ctx->block_map[i];
                            }

                            new_active++;
                        }

                        ctx->active_blocks = new_active;
                    }

                    pthread_barrier_wait(&ctx->workers_barrier);
                }
            }

            pthread_barrier_wait(&ctx->control_barrier);
        }
    }

    return NULL;
}

static void init_block_map(struct sort_context *ctx) {
    size_t chunk_records = ctx->chunk_records;
    int blocks = ctx->blocks;

    if (chunk_records == 0) {
        ctx->active_blocks = 0;
        return;
    }

    size_t base_block_size = chunk_records / (size_t)blocks;
    size_t remainder = chunk_records % (size_t)blocks;

    if (base_block_size == 0) {
        fprintf(stderr, "too many blocks for this chunk\n");
        exit(EXIT_FAILURE);
    }

    size_t offset = 0;
    for (int i = 0; i < blocks; ++i) {
        ctx->block_map[i].offset = offset;
        ctx->block_map[i].size = base_block_size + ((size_t)i < remainder ? 1 : 0);
        ctx->block_map[i].state = 0;
        offset += ctx->block_map[i].size;
    }

    ctx->active_blocks = blocks;
}

static int merge_two_sorted_chunks(int fd,
                                   const struct chunk_info *left,
                                   const struct chunk_info *right) {
    size_t left_bytes = left->records * sizeof(struct index_s);
    size_t right_bytes = right->records * sizeof(struct index_s);
    size_t total_records = left->records + right->records;

    struct index_s *left_buf = malloc(left_bytes);
    struct index_s *right_buf = malloc(right_bytes);
    struct index_s *out_buf = malloc(total_records * sizeof(struct index_s));

    if (!left_buf || !right_buf || !out_buf) {
        perror("malloc merge chunks");
        free(left_buf);
        free(right_buf);
        free(out_buf);
        return -1;
    }

    if (pread(fd, left_buf, left_bytes, left->file_offset) != (ssize_t)left_bytes) {
        perror("pread left chunk");
        free(left_buf);
        free(right_buf);
        free(out_buf);
        return -1;
    }

    if (pread(fd, right_buf, right_bytes, right->file_offset) != (ssize_t)right_bytes) {
        perror("pread right chunk");
        free(left_buf);
        free(right_buf);
        free(out_buf);
        return -1;
    }

    size_t i = 0, j = 0, k = 0;
    while (i < left->records && j < right->records) {
        if (cmp_index(&left_buf[i], &right_buf[j]) <= 0) {
            out_buf[k++] = left_buf[i++];
        } else {
            out_buf[k++] = right_buf[j++];
        }
    }

    while (i < left->records) {
        out_buf[k++] = left_buf[i++];
    }

    while (j < right->records) {
        out_buf[k++] = right_buf[j++];
    }

    if (pwrite(fd, out_buf,
               total_records * sizeof(struct index_s),
               left->file_offset) != (ssize_t)(total_records * sizeof(struct index_s))) {
        perror("pwrite merged chunk");
        free(left_buf);
        free(right_buf);
        free(out_buf);
        return -1;
    }

    free(left_buf);
    free(right_buf);
    free(out_buf);
    return 0;
}

static int merge_sorted_chunks(int fd,
                               struct chunk_info *chunks,
                               size_t chunk_count) {
    while (chunk_count > 1) {
        size_t new_count = 0;

        for (size_t i = 0; i < chunk_count; i += 2) {
            if (i + 1 < chunk_count) {
                if (merge_two_sorted_chunks(fd, &chunks[i], &chunks[i + 1]) != 0) {
                    return -1;
                }

                chunks[new_count].file_offset = chunks[i].file_offset;
                chunks[new_count].records = chunks[i].records + chunks[i + 1].records;
            } else {
                chunks[new_count] = chunks[i];
            }

            new_count++;
        }

        chunk_count = new_count;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    struct cli_args args;

    if (parse_cli(argc, argv, &args) != 0) {
        return EXIT_FAILURE;
    }

    if (validate_cli(&args) != 0) {
        return EXIT_FAILURE;
    }

    int fd = open(args.filename, O_RDWR);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    struct index_hdr_s hdr;
    ssize_t n = pread(fd, &hdr, sizeof(hdr), 0);
    if (n != (ssize_t)sizeof(hdr)) {
        perror("pread header");
        close(fd);
        return EXIT_FAILURE;
    }

    uint64_t records = hdr.records;
    size_t records_per_chunk = args.memsize / sizeof(struct index_s);

    if (records_per_chunk == 0) {
        fprintf(stderr, "memsize is too small for even one record\n");
        close(fd);
        return EXIT_FAILURE;
    }

    size_t max_chunks = (records + records_per_chunk - 1) / records_per_chunk;
    struct chunk_info *chunks = calloc(max_chunks, sizeof(struct chunk_info));
    size_t chunk_count = 0;

    if (!chunks) {
        perror("calloc chunks");
        close(fd);
        return EXIT_FAILURE;
    }

    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize < 0) {
        perror("sysconf");
        free(chunks);
        close(fd);
        return EXIT_FAILURE;
    }

    printf("memsize           = %zu\n", args.memsize);
    printf("blocks            = %d\n", args.blocks);
    printf("threads           = %d\n", args.threads);
    printf("file              = %s\n", args.filename);
    printf("records total     = %llu\n", (unsigned long long)records);
    printf("records per chunk = %zu\n", records_per_chunk);

    struct sort_context ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.fd = fd;
    ctx.memsize = args.memsize;
    ctx.blocks = args.blocks;
    ctx.threads = args.threads;
    ctx.pagesize = pagesize;
    ctx.buf = NULL;
    ctx.chunk_records = 0;
    ctx.block_map = calloc((size_t)args.blocks, sizeof(struct block_info));
    ctx.active_blocks = 0;
    ctx.merge_task_count = 0;
    ctx.phase = 0;
    ctx.done = 0;

    if (!ctx.block_map) {
        perror("calloc block_map");
        free(chunks);
        close(fd);
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&ctx.map_mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        free(ctx.block_map);
        free(chunks);
        close(fd);
        return EXIT_FAILURE;
    }

    if (pthread_barrier_init(&ctx.control_barrier, NULL,
                             (unsigned)args.threads + 1) != 0) {
        perror("pthread_barrier_init control_barrier");
        pthread_mutex_destroy(&ctx.map_mutex);
        free(ctx.block_map);
        free(chunks);
        close(fd);
        return EXIT_FAILURE;
    }

    if (pthread_barrier_init(&ctx.workers_barrier, NULL,
                             (unsigned)args.threads) != 0) {
        perror("pthread_barrier_init workers_barrier");
        pthread_barrier_destroy(&ctx.control_barrier);
        pthread_mutex_destroy(&ctx.map_mutex);
        free(ctx.block_map);
        free(chunks);
        close(fd);
        return EXIT_FAILURE;
    }

    pthread_t *tids = calloc((size_t)args.threads, sizeof(pthread_t));
    struct thread_args *targs = calloc((size_t)args.threads, sizeof(struct thread_args));
    if (!tids || !targs) {
        perror("calloc threads");
        free(tids);
        free(targs);
        pthread_barrier_destroy(&ctx.workers_barrier);
        pthread_barrier_destroy(&ctx.control_barrier);
        pthread_mutex_destroy(&ctx.map_mutex);
        free(ctx.block_map);
        free(chunks);
        close(fd);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < args.threads; ++i) {
        targs[i].thread_id = i;
        targs[i].ctx = &ctx;

        if (pthread_create(&tids[i], NULL, worker_thread, &targs[i]) != 0) {
            perror("pthread_create");
            ctx.done = 1;
            ctx.phase = 2;
            pthread_barrier_wait(&ctx.control_barrier);

            for (int j = 0; j < i; ++j) {
                pthread_join(tids[j], NULL);
            }

            free(tids);
            free(targs);
            pthread_barrier_destroy(&ctx.workers_barrier);
            pthread_barrier_destroy(&ctx.control_barrier);
            pthread_mutex_destroy(&ctx.map_mutex);
            free(ctx.block_map);
            free(chunks);
            close(fd);
            return EXIT_FAILURE;
        }
    }

    off_t data_offset = sizeof(struct index_hdr_s);
    uint64_t processed = 0;

    while (processed < records) {
        uint64_t remaining = records - processed;
        uint64_t chunk_records = remaining;

        if (chunk_records > records_per_chunk) {
            chunk_records = records_per_chunk;
        }

        off_t chunk_file_offset =
            data_offset + (off_t)(processed * sizeof(struct index_s));

        size_t chunk_data_bytes =
            (size_t)chunk_records * sizeof(struct index_s);

        off_t map_offset = chunk_file_offset & ~((off_t)pagesize - 1);
        off_t delta = chunk_file_offset - map_offset;
        size_t map_length = (size_t)delta + chunk_data_bytes;

        printf("chunk: processed=%llu, chunk_records=%llu\n",
               (unsigned long long)processed,
               (unsigned long long)chunk_records);

        void *base = mmap(NULL, map_length,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, map_offset);
        if (base == MAP_FAILED) {
            perror("mmap");
            break;
        }

        ctx.buf = (struct index_s *)((char *)base + delta);
        ctx.chunk_records = (size_t)chunk_records;
        ctx.phase = 1;

        init_block_map(&ctx);

        pthread_barrier_wait(&ctx.control_barrier);  /* старт чанка */
        pthread_barrier_wait(&ctx.control_barrier);  /* конец чанка */

        chunks[chunk_count].file_offset = chunk_file_offset;
        chunks[chunk_count].records = (size_t)chunk_records;
        chunk_count++;

        if (msync(base, map_length, MS_SYNC) < 0) {
            perror("msync");
            munmap(base, map_length);
            break;
        }

        if (munmap(base, map_length) < 0) {
            perror("munmap");
            break;
        }

        ctx.buf = NULL;
        ctx.chunk_records = 0;
        ctx.phase = 0;

        processed += chunk_records;
    }

    ctx.done = 1;
    ctx.phase = 2;
    pthread_barrier_wait(&ctx.control_barrier);

    for (int i = 0; i < args.threads; ++i) {
        pthread_join(tids[i], NULL);
    }

    if (chunk_count > 1) {
        if (merge_sorted_chunks(fd, chunks, chunk_count) != 0) {
            fprintf(stderr, "failed to merge sorted chunks\n");
            free(tids);
            free(targs);
            pthread_barrier_destroy(&ctx.workers_barrier);
            pthread_barrier_destroy(&ctx.control_barrier);
            pthread_mutex_destroy(&ctx.map_mutex);
            free(ctx.block_map);
            free(chunks);
            close(fd);
            return EXIT_FAILURE;
        }
    }

    free(tids);
    free(targs);

    pthread_barrier_destroy(&ctx.workers_barrier);
    pthread_barrier_destroy(&ctx.control_barrier);
    pthread_mutex_destroy(&ctx.map_mutex);
    free(ctx.block_map);
    free(chunks);
    close(fd);

    return EXIT_SUCCESS;
}