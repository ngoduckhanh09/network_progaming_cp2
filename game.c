#include "game.h"

void init_game(GameSession *session)
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            session->board[i][j] = 0;
        }
    }
    session->turn = 1; // Mặc định X đi trước
    session->rematch_count = 0;
    session->is_game_over = 0;
}

int check_win(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int player_id)
{
    int directions[4][2] = {
        {0, 1}, // Ngang
        {1, 0}, // Dọc
        {1, 1}, // Chéo chính
        {1, -1} // Chéo phụ
    };

    for (int d = 0; d < 4; d++)
    {
        int count = 1;
        int dx = directions[d][0];
        int dy = directions[d][1];

        // Duyệt chiều dương
        for (int k = 1; k < 5; k++)
        {
            int r = row + k * dx;
            int c = col + k * dy;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || board[r][c] != player_id)
                break;
            count++;
        }

        // Duyệt chiều âm
        for (int k = 1; k < 5; k++)
        {
            int r = row - k * dx;
            int c = col - k * dy;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || board[r][c] != player_id)
                break;
            count++;
        }

        if (count >= 5)
            return 1; // Thắng
    }
    return 0;
}