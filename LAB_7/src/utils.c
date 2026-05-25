#include "utils.h"
#include <stdio.h>
#include <string.h>

int read_line(char *buf, size_t sz) {
    if (!fgets(buf, sz, stdin)) {
        buf[0] = 0;
        return 0;
    }
    size_t l = strlen(buf);
    if (l && buf[l-1] == '\n') buf[l-1] = 0;
    return 1;
}