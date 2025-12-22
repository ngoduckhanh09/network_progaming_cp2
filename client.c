#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"

// --- TRẠNG THÁI ---
#define STATE_LOGIN 0
#define STATE_WAITING 1
#define STATE_GAME 2

// --- BIẾN TOÀN CỤC (GLOBAL STATE) ---
int sock_fd;
int current_state = STATE_LOGIN;
int board[20][20];    // Bàn cờ lưu tại client
int my_player_id = 0; // Server sẽ cấp (1=X, 2=O)
int my_turn = 0;      // 1: Đến lượt mình, 0: Đợi
int game_running = 1;

// Mutex để tránh việc in màn hình bị lộn xộn
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
void draw_board()
{
    pthread_mutex_lock(&print_lock); // Khóa màn hình

    system("clear"); // Xóa màn hình (Windows dùng "cls")

    printf("=== BAN CO CARO ===\n");
    printf("Ban la: %s\n", (my_player_id == 1) ? "X (Di truoc)" : "O (Di sau)");
    printf("Trang thai: %s\n\n", (my_turn) ? ">> DEN LUOT BAN <<" : "Doi doi thu...");

    // Vẽ tiêu đề cột (0-9) cho gọn, nếu >10 thì lấy số cuối
    printf("   ");
    for (int j = 0; j < 20; j++)
        printf("%d ", j % 10);
    printf("\n");

    for (int i = 0; i < 20; i++)
    {
        printf("%2d ", i); // Số dòng
        for (int j = 0; j < 20; j++)
        {
            char c = '.';
            if (board[i][j] == 1)
                c = 'X';
            else if (board[i][j] == 2)
                c = 'O';

            // Tô màu đơn giản (nếu terminal hỗ trợ)
            if (c == 'X')
                printf("\033[1;31m%c \033[0m", c); // Đỏ
            else if (c == 'O')
                printf("\033[1;32m%c \033[0m", c); // Xanh
            else
                printf("%c ", c);
        }
        printf("\n");
    }
    printf("\nLenh: 'x y' de danh, 'chat [msg]' de chat.\n>> ");
    fflush(stdout);

    pthread_mutex_unlock(&print_lock); // Mở khóa
}
void *receive_thread(void *arg)
{
    Packet pkt;
    while (recv(sock_fd, &pkt, sizeof(Packet), 0) > 0)
    {
        switch (pkt.type)
        {
        case MSG_LOGIN_SUCCESS:
            printf("\nDang nhap thanh cong! Dang tim tran...\n");
            current_state = STATE_WAITING;
            // Tự động gửi yêu cầu tìm trận
            Packet req;
            req.type = MSG_PLAY_REQ;
            send(sock_fd, &req, sizeof(Packet), 0);
            break;

        case MSG_WAIT:
            printf("\nServer: %s\n", pkt.data);
            break;

        case MSG_START:
            // Server gửi ID: 1 là X, 2 là O
            my_player_id = pkt.player_id;
            my_turn = (my_player_id == 1) ? 1 : 0; // X đi trước
            current_state = STATE_GAME;
            memset(board, 0, sizeof(board)); // Xóa bàn cờ cũ
            draw_board();
            break;

        case MSG_MOVE:
            // Cập nhật nước đi của BẤT KỲ AI (cả mình và địch)
            board[pkt.x][pkt.y] = pkt.player_id;

            // Nếu người vừa đánh là mình -> Hết lượt
            // Nếu người vừa đánh là địch -> Đến lượt mình
            if (pkt.player_id == my_player_id)
            {
                my_turn = 0;
            }
            else
            {
                my_turn = 1;
            }
            draw_board();
            break;

        case MSG_END:
            pthread_mutex_lock(&print_lock);
            printf("\n\nKET THUC: %s\nBam Enter de thoat...", pkt.data);
            pthread_mutex_unlock(&print_lock);
            game_running = 0;
            close(sock_fd);
            exit(0);
            break;

        case MSG_CHAT:
            pthread_mutex_lock(&print_lock);
            printf("\n[CHAT]: %s\n>> ", pkt.data);
            fflush(stdout);
            pthread_mutex_unlock(&print_lock);
            break;
        }
    }
    return NULL;
}
int main()
{
    // 1. KẾT NỐI SERVER
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Khong the ket noi Server 127.0.0.1:8888!\n");
        return 1;
    }

    // 2. CHẠY LUỒNG NHẬN
    pthread_t tid;
    pthread_create(&tid, NULL, receive_thread, NULL);

    // 3. LOGIC GỬI (Input Loop)
    char buffer[256];
    while (game_running)
    {
        if (current_state == STATE_LOGIN)
        {
            char u[32], p[32];
            printf("Username: ");
            scanf("%s", u);
            printf("Password: ");
            scanf("%s", p);

            Packet pkt;
            pkt.type = MSG_LOGIN;
            sprintf(pkt.data, "%s %s", u, p);
            send(sock_fd, &pkt, sizeof(Packet), 0);

            sleep(1); // Đợi server phản hồi
        }
        else if (current_state == STATE_WAITING)
        {
            // Chỉ đợi, không làm gì
            sleep(1);
        }
        else if (current_state == STATE_GAME)
        {
            // Đọc lệnh từ người dùng
            // Xóa bộ đệm stdin để tránh trôi lệnh
            fgets(buffer, sizeof(buffer), stdin);

            int x, y;
            if (sscanf(buffer, "%d %d", &x, &y) == 2)
            {
                // Kiểm tra lượt đi tại client cho đỡ spam server
                if (!my_turn)
                {
                    printf("Chua den luot ban!\n");
                    continue;
                }
                if (x < 0 || x >= 20 || y < 0 || y >= 20 || board[x][y] != 0)
                {
                    printf("Nuoc di khong hop le!\n");
                    continue;
                }

                Packet pkt;
                pkt.type = MSG_MOVE;
                pkt.x = x;
                pkt.y = y;
                send(sock_fd, &pkt, sizeof(Packet), 0);

                // Lưu ý: Ta KHÔNG tự điền board[x][y] ở đây
                // Ta đợi Server gửi lại MSG_MOVE xác nhận thì mới vẽ
            }
            else if (strncmp(buffer, "chat ", 5) == 0)
            {
                Packet pkt;
                pkt.type = MSG_CHAT;
                strcpy(pkt.data, buffer + 5);
                // Xóa ký tự xuống dòng
                pkt.data[strcspn(pkt.data, "\n")] = 0;
                send(sock_fd, &pkt, sizeof(Packet), 0);
            }
        }
    }

    close(sock_fd);
    return 0;
}