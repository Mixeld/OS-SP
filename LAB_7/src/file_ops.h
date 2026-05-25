#ifndef FILE_OPS_H
#define FILE_OPS_H

#include "record.h"

int read_record (int fd, int rec_no,       struct record_s *rec);
int write_record(int fd, int rec_no, const struct record_s *rec);
int record_count(int fd);

#endif