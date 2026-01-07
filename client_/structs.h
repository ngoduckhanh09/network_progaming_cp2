#ifndef STRUCTS_H
#define STRUCTS_H

#include <SDL.h>
#include <SDL_ttf.h>

typedef struct
{
	// --- ĐỒ HỌA ---
	SDL_Renderer *renderer;
	SDL_Window *window;
	TTF_Font *font;

	// --- INPUT STATE (Chuột) ---
	int mouseX;
	int mouseY;
	int mouseDown;

	// --- INPUT TEXT (Mới) ---
	// Thay thế inputText cũ bằng 3 biến này:
	char inputUsername[32];
	char inputPassword[32];
	char inputName[32];
	int inputFocus; // 0: Username, 1: Password, 2: Name

	// --- GAME LOGIC ---
	int state;
	int socket;
	int player_id; // 1: X, 2: O
	int turn;	   // 1: Lượt mình, 0: Lượt đối thủ
	int board[20][20];
	char message[100];
	int score;				// Lưu điểm hiện tại
	char leaderboard[1024]; // Lưu nội dung bảng xếp hạng nhận từ server
	char opponentName[32];	// Tên đối thủ
	int opponentScore;		// Điểm đối thủ
} App;

#endif