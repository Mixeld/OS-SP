#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


#define PORT     54321
#define BUF_SIZE 512

/* Прочитать одну строку из сокета (до \n) */
ssize_t read_line(int fd, char *buf, size_t maxlen);

/* Отправить строку в сокет (добавляет \r\n) */
int send_line(int fd, const char *msg);

/* Отправить "+OK сообщение" */
void send_ok(int fd, const char *msg);

/* Отправить "-ERR сообщение" */
void send_err(int fd, const char *msg);

/* Отправить "." — конец многострочного ответа */
void send_end(int fd);

/* Перевести строку в верхний регистр */
void to_upper(char *s);

void cmd_echo(int fd, const char *arg);
void cmd_info(int fd);
void cmd_help(int fd);

int dispatch_command(int fd, char *line);

#endif 