#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"


#define SERVER_IP   "127.0.0.1"


time_t g_start_time;

int is_multiline_command(const char *input) {
    char cmd[BUF_SIZE];

    strncpy(cmd, input, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';

    char *sp = strchr(cmd, ' ');
    if (sp != NULL) {
        *sp = '\0';
    }

    to_upper(cmd);

    if (strcmp(cmd, "HELP") == 0) return 1;
    if (strcmp(cmd, "INFO") == 0) return 1;

    return 0;
}

void receive_response (int fd, int multiline){
    char line [BUF_SIZE];
    ssize_t n;

    n = read_line(fd, line, sizeof(line));

    if (n <= 0){
        fprintf(stderr, "Соединение с сервером отвалилось\n");
        exit(1);
    }

    printf("%s\n", line);

    if (!multiline){
        return;
    }

    while (1){
        n = read_line(fd, line, sizeof(line));
        if (n <= 0) break;

        if(strcmp(line, ".") == 0) break;

        if(line[0] == ' '){
            printf("%s\n", line);
            continue;
        }

        printf("%s\n", line);
        break;

    }
}


int main(void)
{
    //Создаём сокет
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    ///Адрес сервера
    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port   = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &srv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Неверный IP: %s\n", SERVER_IP);
        exit(1);
    }

    //Подключение
    printf("[С] Подключаюсь к %s:%d...\n", SERVER_IP, PORT);

    if (connect(fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("[С] Подключился!\n");

    receive_response(fd, 0);

    char buf[BUF_SIZE];

    while (1) {

        printf("> ");
        fflush(stdout); 

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("\n[С] EOF, выхожу...\n");
            break;
        }

        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[--len] = '\0';

        if (len == 0) continue;

        if (send_line(fd, buf) < 0) {
            fprintf(stderr, "[С] Ошибка отправки\n");
            break;
        }

        int multiline = is_multiline_command(buf);

        receive_response(fd, multiline);

        char upper[BUF_SIZE];
        strncpy(upper, buf, sizeof(upper));
        for (char *p = upper; *p; p++){
            if(*p >= 'a' && *p <= 'z') *p -= 32;
        }
        
        if (strcmp(upper, "QUIT") == 0) break;
    }

    // Закрываем соединение
    close(fd);
    printf("[С] Соединение закрыто\n");

    return 0;
}