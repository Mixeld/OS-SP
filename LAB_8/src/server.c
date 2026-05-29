#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "protocol.h"

time_t g_start_time;
int g_client_count = 0;


typedef struct {
    int fd;
    char ip[INET_ADDRSTRLEN];
    int id;
} client_info_t;



void handle_client(int client_fd, const char *client_ip, int client_id)
{
    char buf[BUF_SIZE];
    ssize_t n;

    printf("[S] (%d) Сеанс с %s начат\n", client_id, client_ip);

    send_ok(client_fd, "Для получения наберите HELP!");

    while (1) {
        n = read_line(client_fd, buf, sizeof(buf));

        if (n < 0) {
            perror("read_line");
            break;
        }
        if (n == 0) {
            printf("[S] Клиент %s отключился\n", client_ip);
            break;
        }

        printf("[S] #%d (%s) >>> \"%s\"\n", client_id, client_ip, buf);

        /* Передаём строку в протокол */
        if (dispatch_command(client_fd, buf) == 0) {
            printf("[S] %s попрощался\n", client_ip);
            break;
        }
    }

    close(client_fd);
    printf("[S] Соединение с %s закрыто\n", client_ip);
}


void *client_trhread(void *arg){
    client_info_t *info = (client_info_t *)arg;

    handle_client(info-> fd, info ->ip, info -> id);
    free(info);

    return NULL;
}

int main (void){
    
    g_start_time = time(NULL);

    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(1);
    }
    printf("[S] Запущен на порту %d\n", PORT);
    printf("[S] Жду подключений...\n");

    while (1){
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
    
    if (client_fd < 0) {
        perror("accept"); 
        continue; 
    }

    g_client_count++;

    client_info_t *info = malloc(sizeof(client_info_t));

    if (info == NULL) {
        fprintf(stderr, "malloc failed\n");
        close(client_fd);
        continue;
    }

    info->fd = client_fd;
    info->id = g_client_count;
    inet_ntop(AF_INET, &client_addr.sin_addr, info->ip, sizeof(info->ip));

    printf("[S] Подключился клиент #%d: %s\n", info->id, info->ip);

    pthread_t tid;

    if (pthread_create(&tid, NULL, client_trhread, info) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(info);
            continue;
    }

    pthread_detach(tid);
    }

    close(server_fd);
    printf("[S] Завершение работы\n");

    return 0;

}