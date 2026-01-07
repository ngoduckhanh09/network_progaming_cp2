#ifndef GAME_H
#define GAME_H

#define BOARD_SIZE 20

// Cấu trúc phiên chơi (Lưu bàn cờ chung giữa 2 người)
typedef struct
{
    int board[BOARD_SIZE][BOARD_SIZE]; // 0: Rỗng, 1: X, 2: O
    int turn;                          // Lượt hiện tại (1 hoặc 2)
    int p1_socket;                     // Socket người chơi 1 (X)
    int p2_socket;                     // Socket người chơi 2 (O)
    int rematch_count;                 // <--- THÊM BIẾN NÀY (0: chưa ai, 1: có 1 người, 2: cả hai)
    int is_game_over;                  // <--- THÊM DÒNG NÀY
} GameSession;

// Khởi tạo bàn cờ mới
void init_game(GameSession *session);

// Kiểm tra thắng thua (Trả về 1 nếu thắng, 0 nếu chưa)
int check_win(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int player_id);

#endif // GAME_H