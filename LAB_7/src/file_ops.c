#include "file_ops.h"
#include <unistd.h>
#include <sys/stat.h>

int read_record(int fd, int rec_no, struct record_s *rec) {
    if (lseek(fd, (off_t)rec_no * REC_SIZE, SEEK_SET) == -1) return -1;
    if (read(fd, rec, REC_SIZE) != (ssize_t)REC_SIZE)        return -1;
    return 0;
}

int write_record(int fd, int rec_no, const struct record_s *rec) {
    if (lseek(fd, (off_t)rec_no * REC_SIZE, SEEK_SET) == -1) return -1;
    if (write(fd, rec, REC_SIZE) != (ssize_t)REC_SIZE)       return -1;
    return 0;
}

int record_count(int fd) {
    struct stat st;
    if (fstat(fd, &st) == -1) return -1;
    return st.st_size / REC_SIZE;
}