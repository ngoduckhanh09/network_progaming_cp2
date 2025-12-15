#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"

// --- ĐỊNH NGHĨA TRẠNG THÁI ---
#define STATE_AUTH 1
#define STATE_WAITING 2
#define STATE_PLAYING 3

// --- BIẾN TOÀN CỤC ---
int current_state = STATE_AUTH;
int sock_fd; // Thống nhất dùng tên này

// --- LUỒNG NHẬN TIN ---
void *receive_thread(void *arg)
{
    Packet pkt;
    while (recv(sock_fd, &pkt, sizeof(Packet), 0) > 0)
    {
        switch (pkt.type)
        {
        case MSG_LOGIN_SUCCESS:
            printf("\n[SERVER] %s\n", pkt.data);
            current_state = STATE_WAITING;

            // Gửi yêu cầu tìm trận ngay khi đăng nhập thành công
            Packet req;
            req.type = MSG_PLAY_REQ;
            send(sock_fd, &req, sizeof(Packet), 0);
            break;

        case MSG_LOGIN_FAIL:
            printf("\n[SERVER] Đăng nhập thất bại: %s\n", pkt.data);
            // Giữ nguyên state AUTH để nhập lại
            break;

        case MSG_WAIT:
            printf("\n[SERVER] %s\n", pkt.data);
            break;

        case MSG_START:
            printf("\n[THÔNG BÁO] Đã tìm thấy đối thủ! Game bắt đầu.\n");
            current_state = STATE_PLAYING;
            break;

        case MSG_MOVE:
            printf("\n[ĐỐI THỦ] Đánh vào ô: %d, %d\n", pkt.x, pkt.y);
            printf(">> ");
            fflush(stdout); // Hiện lại dấu nhắc lệnh
            break;

        case MSG_CHAT:
            printf("\n[CHAT] %s\n", pkt.data);
            printf(">> ");
            fflush(stdout);
            break;
        }
    }
    return NULL;
}

// --- HÀM MAIN ---
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
        printf("Không thể kết nối Server!\n");
        return 1;
    }
    printf("Đã kết nối Server!\n");

    // 2. TẠO LUỒNG NHẬN TIN
    pthread_t tid;
    pthread_create(&tid, NULL, receive_thread, NULL);

    // 3. VÒNG LẶP CHÍNH
    char u[32], p[32];

    while (1)
    {
        // --- TRẠNG THÁI 1: ĐĂNG NHẬP ---
        if (current_state == STATE_AUTH)
        {
            printf("\n=== DANG NHAP ===\n");
            printf("Username: ");
            scanf("%s", u);
            printf("Password: ");
            scanf("%s", p);

            Packet pkt;
            pkt.type = MSG_LOGIN;
            sprintf(pkt.data, "%s %s", u, p);

            // Sửa lỗi tên biến: sockfd -> sock_fd
            send(sock_fd, &pkt, sizeof(Packet), 0);

            // Chờ phản hồi (Hack nhẹ để thread kia kịp cập nhật state)
            sleep(1);
        }

        // --- TRẠNG THÁI 2: ĐANG TÌM TRẬN ---
        else if (current_state == STATE_WAITING)
        {
            printf(".");
            fflush(stdout);
            sleep(1);
        }

        // --- TRẠNG THÁI 3: ĐANG CHƠI ---
        else if (current_state == STATE_PLAYING)
        {
            printf("\n--- LƯỢT CỦA BẠN ---\n");
            printf("Nhập tọa độ (x y) hoặc Chat (c [nội dung]): ");

            char input[100];

            // Xóa bộ đệm bàn phím (tránh trôi lệnh do scanf trước đó)
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;

            fgets(input, sizeof(input), stdin);

            // Xóa ký tự xuống dòng thừa của fgets
            input[strcspn(input, "\n")] = 0;

            // NẾU LÀ CHAT (c hello)
            if (input[0] == 'c' && input[1] == ' ')
            {
                Packet chat_pkt;
                chat_pkt.type = MSG_CHAT;
                strcpy(chat_pkt.data, input + 2); // Bỏ chữ "c "
                send(sock_fd, &chat_pkt, sizeof(Packet), 0);
            }
            // NẾU LÀ ĐÁNH CỜ (10 10)
            else
            {
                int x, y;
                // Sửa lỗi logic: Phải sscanf từ biến input vừa nhập
                if (sscanf(input, "%d %d", &x, &y) == 2)
                {
                    Packet move_pkt;
                    move_pkt.type = MSG_MOVE;
                    move_pkt.x = x;
                    move_pkt.y = y;
                    send(sock_fd, &move_pkt, sizeof(Packet), 0);
                    printf("--> Đã gửi nước đi (%d, %d)\n", x, y);
                }
                else
                {
                    printf("Lệnh không hợp lệ! Hãy nhập lại.\n");
                }
            }
        }
    } // Kết thúc while

    // Đã xóa return 0 ở trong vòng lặp -> Code chạy vô tận đến khi tắt
    close(sock_fd);
    return 0;
}