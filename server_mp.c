#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h" // Protocol chung
#include "auth.h"     // Logic đăng nhập
#include "game.h"     // Logic game

// Cấu trúc quản lý Client
typedef struct
{
    int socket;
    GameSession *session; // Trỏ đến bàn cờ đang chơi
    int player_id;        // 1 (X) hoặc 2 (O)
} ClientInfo;

// Biến toàn cục quản lý ghép cặp
ClientInfo *waiting_client = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Luồng xử lý từng Client
void *client_handler(void *arg)
{
    ClientInfo *me = (ClientInfo *)arg;
    Packet pkt;
    int n;

    while ((n = recv(me->socket, &pkt, sizeof(Packet), 0)) > 0)
    {

        // --- 1. XỬ LÝ ĐĂNG NHẬP (Gọi auth.c) ---
        if (pkt.type == MSG_LOGIN)
        {
            char u[32], p[32];
            sscanf(pkt.data, "%s %s", u, p);

            Packet res;
            if (check_login(u, p))
            { // Gọi hàm từ auth.c
                res.type = MSG_LOGIN_SUCCESS;
                strcpy(res.data, "Dang nhap thanh cong!");
            }
            else
            {
                res.type = MSG_LOGIN_FAIL;
                strcpy(res.data, "Sai mat khau (pass: 123)");
            }
            send(me->socket, &res, sizeof(Packet), 0);
        }

        // --- 2. XỬ LÝ TÌM TRẬN ---
        else if (pkt.type == MSG_PLAY_REQ)
        {
            pthread_mutex_lock(&lock);
            if (waiting_client == NULL)
            {
                // Chưa có ai -> Mình vào hàng đợi
                waiting_client = me;

                Packet res = {.type = MSG_WAIT};
                strcpy(res.data, "Dang tim doi thu...");
                send(me->socket, &res, sizeof(Packet), 0);
            }
            else
            {
                // Đã có người đợi -> Ghép cặp!
                ClientInfo *opponent = waiting_client;
                waiting_client = NULL; // Reset hàng đợi

                // Tạo GameSession mới (Gọi game.c)
                GameSession *new_session = malloc(sizeof(GameSession));
                init_game(new_session);

                // Setup thông tin phiên chơi
                new_session->p1_socket = opponent->socket;
                new_session->p2_socket = me->socket;

                opponent->session = new_session;
                me->session = new_session;

                opponent->player_id = 1; // X
                me->player_id = 2;       // O

                // Gửi thông báo START cho cả 2
                // 1. Gửi cho người đợi (opponent) -> Báo là Player 1 (X)
                Packet pkt1;
                memset(&pkt1, 0, sizeof(Packet)); // Xóa sạch dữ liệu rác
                pkt1.type = MSG_START;
                pkt1.player_id = 1; // <--- QUAN TRỌNG: Gán ID = 1
                strcpy(pkt1.data, "Game Start! Ban la X");
                send(opponent->socket, &pkt1, sizeof(Packet), 0);

                // 2. Gửi cho người mới vào (me) -> Báo là Player 2 (O)
                Packet pkt2;
                memset(&pkt2, 0, sizeof(Packet)); // Xóa sạch dữ liệu rác
                pkt2.type = MSG_START;
                pkt2.player_id = 2; // <--- QUAN TRỌNG: Gán ID = 2
                strcpy(pkt2.data, "Game Start! Ban la O");
                send(me->socket, &pkt2, sizeof(Packet), 0);

                printf("Da ghep cap: %d (X) vs %d (O)\n", opponent->socket, me->socket);
            }
            pthread_mutex_unlock(&lock);
        }

        // --- 3. XỬ LÝ NƯỚC ĐI (Gọi game.c) ---
        else if (pkt.type == MSG_MOVE)
        {
            if (me->session == NULL)
                continue;
            GameSession *s = me->session;

            // Kiểm tra tính hợp lệ
            if (s->turn != me->player_id)
                continue; // Chưa đến lượt
            if (s->board[pkt.x][pkt.y] != 0)
                continue; // Ô đã đánh

            // Update bàn cờ server
            s->board[pkt.x][pkt.y] = me->player_id;

            // Kiểm tra thắng thua (Gọi hàm từ game.c)
            int win = check_win(s->board, pkt.x, pkt.y, me->player_id);

            // Gửi update cho cả 2 người
            Packet update = pkt;
            update.player_id = me->player_id;
            send(s->p1_socket, &update, sizeof(Packet), 0);
            send(s->p2_socket, &update, sizeof(Packet), 0);

            if (win)
            {
                Packet end = {.type = MSG_END};
                sprintf(end.data, "Nguoi choi %s chien thang!", (me->player_id == 1) ? "X" : "O");
                send(s->p1_socket, &end, sizeof(Packet), 0);
                send(s->p2_socket, &end, sizeof(Packet), 0);
                // Logic dọn dẹp bộ nhớ game session nên đặt ở đây
            }
            else
            {
                // Đổi lượt
                s->turn = (s->turn == 1) ? 2 : 1;
            }
        }

        else if (pkt.type == MSG_CHAT)
        {
            if (me->session)
            {
                int dest = (me->socket == me->session->p1_socket) ? me->session->p2_socket : me->session->p1_socket;
                send(dest, &pkt, sizeof(Packet), 0);
            }
        }

        else if (pkt.type == MSG_QUIT)
        {
            break;
        }
    }

    // --- Cleanup khi client thoát ---
    close(me->socket);
    free(me);
    return NULL;
}

int main()
{
    int server_fd;
    struct sockaddr_in address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }

    listen(server_fd, 4);
    printf("Server listening on port 8888...\n");

    while (1)
    {
        int new_sock = accept(server_fd, NULL, NULL);
        if (new_sock < 0)
            continue;

        ClientInfo *new_client = malloc(sizeof(ClientInfo));
        new_client->socket = new_sock;
        new_client->session = NULL;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, new_client);
        pthread_detach(tid);
    }
    return 0;
}