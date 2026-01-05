/*
 * Copyright (C) 2015-2018,2022 Parallel Realities. All rights reserved.
 */

#ifndef DEFS_H
#define DEFS_H

#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 600

#define CELL_SIZE 30
#define BOARD_SIZE 20

// CÁC TRẠNG THÁI CỦA GAME
#define STATE_MENU 0           // Menu chính (3 nút)
#define STATE_LOGIN_INPUT 1    // Màn hình nhập đăng nhập
#define STATE_REGISTER_INPUT 2 // Màn hình nhập đăng ký
#define STATE_WAITING 3        // Đợi tìm trận
#define STATE_GAME 4           // Đang chơi
#define STATE_GAME_OVER 5      // Kết thúc
#define STATE_LOBBY 6          // <--- THÊM
#define STATE_LEADERBOARD 7    // <--- THÊM DÒNG NÀY
#endif