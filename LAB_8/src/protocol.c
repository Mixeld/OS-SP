/*
 * protocol.c — реализация протокола
 *
 * Здесь живут все команды и вспомогательные функции.
 * Чтобы добавить новую команду — смотри инструкцию
 * в конце файла.
 */

#include "protocol.h"

/* Нужен для INFO — время запуска сервера.
 * Устанавливается в server.c */
extern time_t g_start_time;

/* ═══════════════════════════════════════════════════════
 *  Функции для работы с сокетом
 * ═══════════════════════════════════════════════════════ */

ssize_t read_line(int fd, char *buf, size_t maxlen) {
    size_t  i = 0;
    char    c;
    ssize_t n;

    while (i < maxlen - 1) {
        n = read(fd, &c, 1);
        if (n < 0) return -1;
        if (n == 0) break;
        if (c == '\n') break;
        if (c == '\r') continue;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

int send_line(int fd, const char *msg) {
    char    buf[BUF_SIZE];
    int     len = snprintf(buf, sizeof(buf), "%s\r\n", msg);
    size_t  sent = 0;
    ssize_t n;

    while (sent < (size_t)len) {
        n = write(fd, buf + sent, len - sent);
        if (n < 0) return -1;
        sent += n;
    }
    return 0;
}

// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ

void send_ok(int fd, const char *msg) {
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), "+OK %s", msg);
    send_line(fd, buf);
}

void send_err(int fd, const char *msg) {
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), "-ERR %s", msg);
    send_line(fd, buf);
}

void send_end(int fd) {
    send_line(fd, ".");
}

void to_upper(char *s) {
    for (; *s; s++) {
        if (*s >= 'a' && *s <= 'z') {
            *s -= 32;
        }
    }
}

/*

Команды протокола

*/

// ECHO
void cmd_echo(int fd, const char *arg) {
    if (arg == NULL || *arg == '\0') {
        send_err(fd, "ECHO requires an argument");
        return;
    }
    send_ok(fd, arg);
}

// INFO
void cmd_info(int fd) {
    char buf[BUF_SIZE];
    time_t now = time(NULL);
    long uptime = (long)(now - g_start_time);

    char time_str[64];
    struct tm *tm = localtime(&g_start_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);

    send_line(fd, "+OK Server information:");

    snprintf(buf, sizeof(buf), "  Server    : Lab8 TCP Server");
    send_line(fd, buf);

    snprintf(buf, sizeof(buf), "  Port      : %d", PORT);
    send_line(fd, buf);

    snprintf(buf, sizeof(buf), "  Started   : %s", time_str);
    send_line(fd, buf);

    snprintf(buf, sizeof(buf), "  Uptime    : %ld sec", uptime);
    send_line(fd, buf);

    snprintf(buf, sizeof(buf), "  PID       : %d", getpid());
    send_line(fd, buf);

    send_end(fd);
}

//  HELP
void cmd_help(int fd) {
    send_line(fd, "+OK Available commands:");
    send_line(fd, "  ECHO <text>  - echo text back");
    send_line(fd, "  INFO         - server information");
    send_line(fd, "  HELP         - this help");
    send_line(fd, "  QUIT         - close connection");
    send_end(fd);
}

// Диспетчер команд

int dispatch_command(int fd, char *line){
    char *cmd = line;
    while (*cmd == ' ') cmd++;

    if (*cmd == '\0') return 1;

    char *arg = NULL;
    char *sp = strchr(cmd, ' ');
    if (sp != NULL) {
        *sp = '\0';
        arg = sp + 1;
        while (*arg == ' ') arg++;
    }

    to_upper(cmd);

    if (strcmp(cmd, "ECHO") == 0) {
        cmd_echo(fd, arg);
    }
    else if (strcmp(cmd, "INFO") == 0) {
        cmd_info(fd);
    }
    else if (strcmp(cmd, "HELP") == 0) {
        cmd_help(fd);
    }
    else if (strcmp(cmd, "QUIT") == 0) {
        send_ok(fd, "Goodbye!");
        return 0;
    }
    else {
        char errmsg[BUF_SIZE];
        snprintf(errmsg, sizeof(errmsg), "Unknown command: %s", cmd);
        send_err(fd, errmsg);
    }

    return 1;
}
