
#include "record.h"
#include "commands.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
    int fd = open(FILENAME, O_RDWR | O_CREAT, 0644);
    if (fd == -1) { perror("open"); return 1; }

    printf("=== Студенты (PID=%d) ===\n", getpid());

    char line[128];
    while (1) {
        printf("\nКоманды: LST | GET n | PUT n | ADD | QUIT\n> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        char cmd[16]; int n = -1;
        int k = sscanf(line, "%15s %d", cmd, &n);
        if (k < 1) continue;

        if      (!strcasecmp(cmd, "LST"))           cmd_list(fd);
        else if (!strcasecmp(cmd, "GET") && k == 2) cmd_get(fd, n);
        else if (!strcasecmp(cmd, "PUT") && k == 2) cmd_put(fd, n);
        else if (!strcasecmp(cmd, "ADD"))           cmd_add(fd);
        else if (!strcasecmp(cmd, "QUIT"))          break;
        else printf("Неизвестная команда\n");
    }
    close(fd);
    return 0;
}