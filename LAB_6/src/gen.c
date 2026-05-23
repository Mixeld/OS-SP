#include "index.h"

static void usage(const char *prog){
    fprintf(stderr, "Usage: %s <records> <filename>\n", prog);
    exit(EXIT_FAILURE);  
}

static double random_mjd (double mjd_min, double mjd_max){
    double r = (double)rand() / (double)RAND_MAX;
    return mjd_min + r * (mjd_max - mjd_min);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argv[0]);
    }

    uint64_t records = strtoull(argv[1], NULL, 10); // Переводим строку в  unsigned long long 
    const char *filename = argv[2];

    if (records == 0 || (records % 256) != 0) {
        fprintf(stderr, "records must be > 0 and multiple of 256\n");
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    // инициализация генератора случайных чисел
    srand((unsigned)time(NULL));

    struct index_hdr_s hdr;
    hdr.records = records;

    // записываем заголовок
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) {
        perror("fwrite header");
        fclose(fp);
        return EXIT_FAILURE;
    }

    struct index_s rec;
    double mjd_min = 15020.0;
    double mjd_max = 60000.0;

    for (uint64_t i = 0; i < records; ++i) {
        rec.time_mark = random_mjd(mjd_min, mjd_max);
        rec.recno = i + 1; 

        if (fwrite(&rec, sizeof(rec), 1, fp) != 1) {
            perror("fwrite record");
            fclose(fp);
            return EXIT_FAILURE;
        }
    }

    fclose(fp);
    return EXIT_SUCCESS;
}