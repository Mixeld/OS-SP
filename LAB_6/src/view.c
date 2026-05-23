#include "index.h"

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <filename>\n", prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }

    const char *filename = argv[1];

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    struct index_hdr_s hdr;

    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
        perror("fread header");
        fclose(fp);
        return EXIT_FAILURE;
    }

    printf("records = %llu\n", (unsigned long long)hdr.records);

    struct index_s rec;
    uint64_t i = 0;

    while (i < hdr.records && fread(&rec, sizeof(rec), 1, fp) == 1) {
        printf("%10llu: time_mark = %.10f, recno = %llu\n",
               (unsigned long long)i,
               rec.time_mark,
               (unsigned long long)rec.recno);
        ++i;
    }

    if (i != hdr.records) {
        fprintf(stderr, "warning: expected %llu records, read %llu\n",
                (unsigned long long)hdr.records,
                (unsigned long long)i);
    }

    fclose(fp);
    return EXIT_SUCCESS;
}