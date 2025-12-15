#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h> //socket
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h>
#include "protocol.h"
// client
typedef struct
{
    int socket;
    int opponent_socket;
} ClientInfo;
ClientInfo *waiting_client = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // lock protect waiting_client
// check login
int check_login(char *user, char *pass)
{
    if (strcmp(pass, "123") == 0)
    {
        return 1;
    }
    return 0;
}
// function handle client
void *client_handler(void *arg)
{
    ClientInfo *me = (ClientInfo *)arg;
    me->opponent_socket = -1; // mac dinh chua co doi thu
    Packet pkt;
    int n;
    while ((n = recv(me->socket, &pkt, sizeof(Packet), 0)) > 0)
    {
        // login
        if (pkt.type == MSG_LOGIN)
        {
            char u[32], p[32]; //"user pass"
            sscanf(pkt.data, "%s %s", u, p);
            Packet res;
            if (check_login(u, p))
            {
                res.type = MSG_LOGIN_SUCCESS;
                strcpy(res.data, "successful");
            }
            else
            {
                res.type = MSG_LOGIN_FAIL;
                strcpy(res.data, "wrong, pass = 123");
            }
            send(me->socket, &res, sizeof(Packet), 0);
        }
        else if (pkt.type == MSG_PLAY_REQ)
        {
            pthread_mutex_lock(&lock);
            if (waiting_client == NULL)
            {
                // nobody
                waiting_client = me;
                printf("CLient %d waiting ...\n", me->socket);
                // send noti to client
                Packet pkt;
                pkt.type = MSG_WAIT;
                strcpy(pkt.data, "dang tim");
                send(me->socket, &pkt, sizeof(Packet), 0);
            }
            else
            {
                // ghep cap
                ClientInfo *opponent = waiting_client;
                me->opponent_socket = opponent->socket;
                opponent->opponent_socket = me->socket;
                waiting_client = NULL; // reset queue
                printf("da ghep cap %d vs %d\n", me->socket, opponent->socket);
                // send noti 2 player
                Packet pkt;
                pkt.type = MSG_START;
                strcpy(pkt.data, "Game start");
                send(me->socket, &pkt, sizeof(Packet), 0);
                send(opponent->socket, &pkt, sizeof(Packet), 0);
            }
            pthread_mutex_unlock(&lock);
        }
        else if (pkt.type == MSG_MOVE || pkt.type == MSG_CHAT)
        {
            /* code */
            if (me->opponent_socket != -1)
            {
                // Chuyển tiếp gói tin sang cho đối thủ
                send(me->opponent_socket, &pkt, sizeof(Packet), 0);
                printf("Chuyen tin from %d -> %d\n", me->socket, me->opponent_socket);
            }
            else
            {
                // Nếu chưa có đối thủ mà cố tình gửi Move -> Báo lỗi hoặc lờ đi
                Packet err;
                err.type = MSG_CHAT;
                strcpy(err.data, "Chua co doi thu!");
                send(me->socket, &err, sizeof(Packet), 0);
            }
        }
        else if (pkt.type == MSG_QUIT)
        {
            break;
        }
    }

    // logic

    // recv data & send

    // clear & disconnect
    printf("Client %d thoat.\n", me->socket);
    pthread_mutex_lock(&lock);
    if (waiting_client == me)
    {
        waiting_client = NULL;
    }
    pthread_mutex_unlock(&lock);

    // Nếu đang chơi dở -> Báo đối thủ thắng
    if (me->opponent_socket != -1)
    {
        // Bạn tự thêm logic gửi MSG_END cho đối thủ ở đây
    }

    close(me->socket);
    free(me);
    return NULL;
}
int main()
{
    int server_fd;
    struct sockaddr_in address;
    // create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("create socket fail");
        exit(1);
    } // catch error
    printf("create socket ok, %d\n", server_fd);
    // config socket
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Lỗi setsockopt");
        exit(1);
    }
    // bind port 8888
    address.sin_family = AF_INET;         // ipv4
    address.sin_addr.s_addr = INADDR_ANY; // accept any ip
    address.sin_port = htons(8888);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind error");
        exit(1);
    } // catch error
    printf("binding ok on 8888\n");
    // listening
    if (listen(server_fd, 4) < 0)
    {
        perror("listen error");
        exit(1);
    }
    printf("server listening on 8888 ");
    while (1)
    {
        /* code */
        int new_sock = accept(server_fd, NULL, NULL);
        if (new_sock < 0)
        {
            continue;
        }
        // create malloc
        ClientInfo *new_client = malloc(sizeof(ClientInfo));
        new_client->socket = new_sock;
        // create thread
        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, new_client);
        pthread_detach(tid);
    }
    return 0;
}