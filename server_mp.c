#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h"
#include "auth.h"
#include "game.h"
#include <netdb.h>      // Để dùng gethostbyname
#define MAX_CLIENTS 100 // Tối đa 100 người online cùng lúc

// Cấu trúc quản lý Client
typedef struct
{
    int socket;
    GameSession *session;
    int player_id;
    char username[32]; // Tên đăng nhập
} ClientInfo;

// Biến toàn cục
ClientInfo *waiting_client = NULL;       // Người đang đợi tìm trận
ClientInfo *online_clients[MAX_CLIENTS]; // Danh sách người đang online
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// --- HÀM HỖ TRỢ QUẢN LÝ ONLINE ---
// Kiểm tra xem username này đã online chưa
int is_user_online(char *username)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (online_clients[i] != NULL && strcmp(online_clients[i]->username, username) == 0)
        {
            return 1; // Đang online
        }
    }
    return 0; // Chưa online
}

// Thêm người vào danh sách online
void add_online_user(ClientInfo *client)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (online_clients[i] == NULL)
        {
            online_clients[i] = client;
            break;
        }
    }
}

// Xóa người khỏi danh sách online
void remove_online_user(ClientInfo *client)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (online_clients[i] == client)
        {
            online_clients[i] = NULL;
            break;
        }
    }
}
void handle_player_quit(ClientInfo *me)
{
    if (me->session == NULL)
        return;

    GameSession *s = me->session;
    int opp_sock = (me->socket == s->p1_socket) ? s->p2_socket : s->p1_socket;

    // Tìm thông tin đối thủ trong danh sách online
    ClientInfo *opponent = NULL;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (online_clients[i] && online_clients[i]->socket == opp_sock)
        {
            opponent = online_clients[i];
            break;
        }
    }

    // TRƯỜNG HỢP 1: Game đang diễn ra (Chưa ai thắng) -> XỬ THUA
    if (s->is_game_over == 0 && opponent != NULL)
    {
        // 1. Cập nhật DB: Đối thủ thắng, Mình thua
        int old_winner_score = get_user_score(opponent->username);
        int old_loser_score = get_user_score(me->username);

        // 2. Tự tính toán điểm MỚI (Tránh việc đọc file bị chậm)
        int new_winner_score = old_winner_score + 10;
        int new_loser_score = old_loser_score - 10;
        update_game_result(opponent->username, me->username);

        // 2. Gửi thông báo THẮNG cho đối thủ
        Packet end_pkt;
        memset(&end_pkt, 0, sizeof(Packet));
        end_pkt.type = MSG_END;
        end_pkt.score = new_winner_score;         // Điểm người nhận (Thắng)
        end_pkt.opponent_score = new_loser_score; // Điểm người kia (Thua)
        sprintf(end_pkt.data, "Doi thu da thoat. BAN THANG! (+10). Tong: %d", new_winner_score);
        send(opponent->socket, &end_pkt, sizeof(Packet), 0);

        Packet lose_pkt;
        memset(&lose_pkt, 0, sizeof(Packet));
        lose_pkt.type = MSG_END;
        lose_pkt.score = new_loser_score;           // Điểm người nhận (Thua)
        lose_pkt.opponent_score = new_winner_score; // Điểm người kia (Thắng)
        sprintf(lose_pkt.data, "Ban da thoat va bi xu thua (-10). Tong: %d", new_loser_score);
        send(me->socket, &lose_pkt, sizeof(Packet), 0);
        // Đánh dấu game đã xong (để tránh trừ điểm lần nữa nếu gọi lại)
        s->is_game_over = 1;
    }
    // TRƯỜNG HỢP 2: Game đã xong -> Chỉ báo đối thủ biết
    else if (opponent != NULL)
    {
        Packet noti;
        noti.type = MSG_CHAT;
        strcpy(noti.data, "Doi thu da roi phong.");
        send(opponent->socket, &noti, sizeof(Packet), 0);
    }

    // Dọn dẹp session
    if (opponent)
        opponent->session = NULL;
    me->session = NULL;
    free(s);
}
// Luồng xử lý từng Client
void *client_handler(void *arg)
{
    ClientInfo *me = (ClientInfo *)arg;
    Packet pkt;
    int n;

    while ((n = recv(me->socket, &pkt, sizeof(Packet), 0)) > 0)
    {
        // --- 1. XỬ LÝ ĐĂNG NHẬP ---
        if (pkt.type == MSG_LOGIN)
        {
            char u[32], p[32];
            sscanf(pkt.data, "%s %s", u, p);

            Packet res;

            pthread_mutex_lock(&lock); // Khóa để kiểm tra online an toàn
            if (is_user_online(u))
            {
                res.type = MSG_LOGIN_FAIL;
                strcpy(res.data, "Tai khoan nay dang online!");
            }
            else if (check_login(u, p))
            {
                res.type = MSG_LOGIN_SUCCESS;
                // Lấy điểm hiện tại
                int score = get_user_score(u);

                // Mẹo: Dùng trường 'x' của Packet để gửi điểm số (đỡ phải parse chuỗi)
                res.score = score; // <--- Sửa: Gán điểm vào trường score

                sprintf(res.data, "Login OK! Diem: %d", score);

                strcpy(me->username, u);
                add_online_user(me);
            }
            else
            {
                res.type = MSG_LOGIN_FAIL;
                strcpy(res.data, "Sai tai khoan hoac mat khau!");
            }
            pthread_mutex_unlock(&lock);

            send(me->socket, &res, sizeof(Packet), 0);
        }
        // --- XỬ LÝ ĐĂNG KÝ ---
        else if (pkt.type == MSG_REGISTER)
        {
            char u[32], p[32], n[32];
            if (sscanf(pkt.data, "%s %s %s", u, p, n) == 3)
            {
                Packet res;
                if (register_user(u, p, n))
                {
                    res.type = MSG_LOGIN_SUCCESS;
                    strcpy(res.data, "Dang ky thanh cong! Vui long dang nhap.");
                    // Đăng ký xong chưa tự đăng nhập, user phải login lại
                }
                else
                {
                    res.type = MSG_LOGIN_FAIL;
                    strcpy(res.data, "Tai khoan da ton tai!");
                }
                send(me->socket, &res, sizeof(Packet), 0);
            }
        }
        // --- CÁC XỬ LÝ KHÁC (GIỮ NGUYÊN) ---
        else if (pkt.type == MSG_CANCEL_FIND)
        {
            pthread_mutex_lock(&lock);
            if (waiting_client == me)
            {
                waiting_client = NULL;
                printf("Client %s da huy tim tran.\n", me->username);
            }
            pthread_mutex_unlock(&lock);
        }
        else if (pkt.type == MSG_PLAY_REQ)
        {
            pthread_mutex_lock(&lock);
            if (waiting_client == NULL)
            {
                waiting_client = me;
                Packet res = {.type = MSG_WAIT};
                strcpy(res.data, "Dang tim doi thu...");
                send(me->socket, &res, sizeof(Packet), 0);
            }
            else
            {
                ClientInfo *opponent = waiting_client;
                waiting_client = NULL;

                GameSession *new_session = malloc(sizeof(GameSession));
                init_game(new_session);

                new_session->p1_socket = opponent->socket;
                new_session->p2_socket = me->socket;

                opponent->session = new_session;
                me->session = new_session;

                opponent->player_id = 1; // X
                me->player_id = 2;       // O
                int score_opp = get_user_score(opponent->username);
                int score_me = get_user_score(me->username);

                // 2. Gửi cho Opponent (Người đợi): Gửi tên và điểm của người vừa vào (ME)
                Packet pkt1;
                memset(&pkt1, 0, sizeof(Packet));
                pkt1.type = MSG_START;
                pkt1.player_id = 1;
                // Format chuẩn: "Tên Điểm" (Bỏ chữ "Doi thu:" đi để client dễ đọc)
                sprintf(pkt1.data, "%s %d", me->username, score_me);
                send(opponent->socket, &pkt1, sizeof(Packet), 0);

                // 3. Gửi cho Me (Người vừa vào): Gửi tên và điểm của người đang đợi (OPPONENT)
                Packet pkt2;
                memset(&pkt2, 0, sizeof(Packet));
                pkt2.type = MSG_START;
                pkt2.player_id = 2;
                // Format chuẩn: "Tên Điểm"
                sprintf(pkt2.data, "%s %d", opponent->username, score_opp);
                send(me->socket, &pkt2, sizeof(Packet), 0);
                printf("Tran dau: %s (X) vs %s (O)\n", opponent->username, me->username);
            }
            pthread_mutex_unlock(&lock);
        }
        else if (pkt.type == MSG_MOVE)
        {
            if (me->session == NULL)
                continue;
            GameSession *s = me->session;

            // Kiểm tra lượt và ô trống
            if (s->turn != me->player_id)
                continue;
            if (s->board[pkt.x][pkt.y] != 0)
                continue;
            s->board[pkt.x][pkt.y] = me->player_id;
            // 1. Cập nhật bàn cờ
            pkt.player_id = me->player_id;
            // ----------------------------------

            // 2. Gửi ngay nước đi này cho đối thủ

            // 2. QUAN TRỌNG: Gửi ngay nước đi này cho đối thủ (dù thắng hay chưa)
            // Để màn hình đối thủ hiện nước cờ này lên trước khi hiện bảng kết quả.
            int opponent_sock = (me->player_id == 1) ? s->p2_socket : s->p1_socket;
            send(opponent_sock, &pkt, sizeof(Packet), 0);

            // 3. Kiểm tra thắng thua
            int win = check_win(s->board, pkt.x, pkt.y, me->player_id);
            if (win)
            {
                // --- XỬ LÝ KHI THẮNG ---

                // Tìm thông tin người thua để trừ điểm
                // (opponent_sock chính là loser_sock)
                char *loser_name = NULL;
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (online_clients[i] && online_clients[i]->socket == opponent_sock)
                    {
                        loser_name = online_clients[i]->username;
                        break;
                    }
                }

                if (loser_name)
                {
                    s->is_game_over = 1; // <--- THÊM DÒNG NÀY: Đánh dấu game đã có kết quả
                    update_game_result(me->username, loser_name);
                    int old_winner_score = get_user_score(me->username);
                    int old_loser_score = get_user_score(loser_name);

                    // 2. Tính điểm MỚI
                    int new_winner_score = old_winner_score + 10;
                    int new_loser_score = old_loser_score - 10;
                    // Gửi MSG_END cho người thắng
                    update_game_result(me->username, loser_name);

                    // 4. Gửi MSG_END cho NGƯỜI THẮNG
                    Packet end_winner;
                    memset(&end_winner, 0, sizeof(Packet));
                    end_winner.type = MSG_END;
                    end_winner.score = new_winner_score;         // <--- Điểm mới của mình
                    end_winner.opponent_score = new_loser_score; // <--- [QUAN TRỌNG] Điểm mới của đối thủ
                    sprintf(end_winner.data, "BAN THANG! (+10 diem). Tong: %d", new_winner_score);
                    send(me->socket, &end_winner, sizeof(Packet), 0);

                    // 5. Gửi MSG_END cho NGƯỜI THUA
                    Packet end_loser;
                    memset(&end_loser, 0, sizeof(Packet));
                    end_loser.type = MSG_END;
                    end_loser.score = new_loser_score;           // <--- Điểm mới của mình
                    end_loser.opponent_score = new_winner_score; // <--- [QUAN TRỌNG] Điểm mới của đối thủ
                    sprintf(end_loser.data, "%s da thang. \nBan bi tru 10 diem. Tong: %d", me->username, new_loser_score);
                    send(opponent_sock, &end_loser, sizeof(Packet), 0);
                }

                // (Tùy chọn) Reset session hoặc dọn dẹp ở đây nếu cần
            }
            else
            {
                // --- NẾU CHƯA THẮNG ---
                // Chỉ cần đổi lượt (Nước đi đã gửi ở bước 2 rồi)
                s->turn = (me->player_id == 1) ? 2 : 1;
            }
        }
        else if (pkt.type == MSG_LEADERBOARD)
        {
            Packet res;
            res.type = MSG_LEADERBOARD; // Tạm dùng msg CHAT để hiển thị text
            get_leaderboard(res.data);
            send(me->socket, &res, sizeof(Packet), 0);
        }
        else if (pkt.type == MSG_QUIT)
        {
            break;
        }
        // Trong server_mp.c, thêm vào chuỗi if-else xử lý packet
        else if (pkt.type == MSG_PLAY_AGAIN)
        {
            if (me->session == NULL)
            {
                Packet err;
                memset(&err, 0, sizeof(Packet));
                err.type = MSG_CHAT; // Dùng MSG_CHAT để hiện thông báo lên màn hình client
                strcpy(err.data, "Doi thu da thoat, khong the tai dau!");
                send(me->socket, &err, sizeof(Packet), 0);
                continue; // Bỏ qua các lệnh dưới
            }

            pthread_mutex_lock(&lock);
            GameSession *s = me->session;

            s->rematch_count++; // Tăng số người đồng ý

            if (s->rematch_count >= 2) // Cả 2 đều đồng ý
            {
                // 1. Reset game
                memset(s->board, 0, sizeof(s->board));
                s->turn = 1;          // Mặc định người chơi 1 đi trước (hoặc random tùy bạn)
                s->rematch_count = 0; // Reset đếm cho ván sau
                s->is_game_over = 0;
                // 2. Thông báo bắt đầu lại cho cả 2
                Packet startPkt;
                startPkt.type = MSG_START;

                // Gửi cho P1
                startPkt.player_id = 1;
                strcpy(startPkt.data, "Ván mới bắt đầu! Bạn là X");
                send(s->p1_socket, &startPkt, sizeof(Packet), 0);

                // Gửi cho P2
                startPkt.player_id = 2;
                strcpy(startPkt.data, "Ván mới bắt đầu! Bạn là O");
                send(s->p2_socket, &startPkt, sizeof(Packet), 0);
            }
            else
            {
                // Nếu mới chỉ có mình mình bấm, thông báo đợi
                Packet waitPkt;
                waitPkt.type = MSG_CHAT; // Dùng MSG_CHAT để hiện thông báo nhỏ
                strcpy(waitPkt.data, "Dang doi doi thu chap nhan...");
                send(me->socket, &waitPkt, sizeof(Packet), 0);

                // (Tùy chọn) Báo cho đối thủ biết
                int opp_sock = (me->socket == s->p1_socket) ? s->p2_socket : s->p1_socket;
                strcpy(waitPkt.data, "Doi thu muon tai dau!");
                send(opp_sock, &waitPkt, sizeof(Packet), 0);
            }
            pthread_mutex_unlock(&lock);
        }
        else if (pkt.type == MSG_LOGOUT)
        {
            pthread_mutex_lock(&lock);

            // 1. Xóa khỏi danh sách Online
            remove_online_user(me);

            // 2. Nếu đang treo máy tìm trận thì hủy luôn
            if (waiting_client == me)
            {
                waiting_client = NULL;
                printf("User %s da huy tim tran (Logout).\n", me->username);
            }

            // 3. Xóa thông tin session/user gắn với socket này
            // Để socket này trở thành socket "vô danh" (như lúc mới kết nối)
            memset(me->username, 0, 32);
            me->session = NULL; // Đảm bảo không còn dính session nào

            pthread_mutex_unlock(&lock);

            printf("Socket %d da dang xuat (Logout).\n", me->socket);
            // LƯU Ý: KHÔNG break; để vòng lặp tiếp tục lắng nghe (cho lần đăng nhập sau)
        }
        else if (pkt.type == MSG_LEAVE_ROOM)
        {
            pthread_mutex_lock(&lock);
            handle_player_quit(me);
            pthread_mutex_unlock(&lock);
        }
    }

    // --- CLEANUP KHI NGẮT KẾT NỐI ---
    pthread_mutex_lock(&lock);

    // 1. Xóa khỏi danh sách online
    remove_online_user(me);
    printf("Client %s da ngat ket noi.\n", me->username);

    // 2. Xóa khỏi hàng đợi nếu đang đợi
    if (waiting_client == me)
    {
        waiting_client = NULL;
    }

    // 3. Xử lý nếu đang trong trận (Thắng/Thua xử lý sau)
    // if (me->session) { ... }
    handle_player_quit(me);
    pthread_mutex_unlock(&lock);

    close(me->socket);
    free(me);
    return NULL;
}

int main()
{
    // Khởi tạo danh sách online
    for (int i = 0; i < MAX_CLIENTS; i++)
        online_clients[i] = NULL;

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
        memset(new_client->username, 0, 32); // Xóa tên cũ

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, new_client);
        pthread_detach(tid);
    }
    return 0;
}