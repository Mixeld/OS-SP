
#include "locks.h"
#include "record.h"
#include <fcntl.h>
#include <unistd.h>

int lock_record(int fd, int rec_no, short type) {
    struct flock fl;
    fl.l_type   = type;
    fl.l_whence = SEEK_SET;
    fl.l_start  = (off_t)rec_no * REC_SIZE;
    fl.l_len    = REC_SIZE;
    fl.l_pid    = 0;   /* для OFD должно быть 0 */
    return fcntl(fd, F_OFD_SETLKW, &fl);
}