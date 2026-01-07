#include "common.h"
#include "init.h"
#include "input.h"
#include "draw.h"
#include "protocol.h"
#include <pthread.h>
App app;

void *receive_thread(void *arg)
{
	Packet pkt;
	while (recv(app.socket, (char *)&pkt, sizeof(Packet), 0) > 0)
	{
		switch (pkt.type)
		{
		case MSG_LOGIN_SUCCESS:
			if (app.state == STATE_REGISTER_INPUT)
			{
				strcpy(app.message, "Dang ky thanh cong! Hay dang nhap.");
				app.state = STATE_LOGIN_INPUT;
				// Reset form
				memset(app.inputUsername, 0, 32);
				memset(app.inputPassword, 0, 32);
				app.inputFocus = 0;
			}
			else
			{
				app.state = STATE_LOBBY;
				strcpy(app.message, "");
				app.score = pkt.score;
			}
			break;
		case MSG_LOGIN_FAIL:
			strcpy(app.message, pkt.data);
			break;
		case MSG_START:
			app.player_id = pkt.player_id;
			app.turn = 1; // X luôn đi trước
			app.state = STATE_GAME;
			memset(app.board, 0, sizeof(app.board));
			memset(app.message, 0, sizeof(app.message));
			sscanf(pkt.data, "%s %d", app.opponentName, &app.opponentScore);
			break;
		case MSG_MOVE:
			app.board[pkt.x][pkt.y] = pkt.player_id;
			app.turn = (pkt.player_id == 1) ? 2 : 1;
			break;
		case MSG_END:
			// strcpy(app.message, pkt.data);
			// app.state = STATE_GAME_OVER;
			// app.score = pkt.score;
			// app.opponentScore = pkt.opponent_score; // <--- [MỚI] Cập nhật điểm đối thủ
			app.score = pkt.score;
			app.opponentScore = pkt.opponent_score;

			// NẾU ĐANG Ở SẢNH (Do vừa bấm thoát chủ động):
			if (app.state == STATE_LOBBY)
			{
				// Chỉ hiện thông báo nhỏ (nếu cần), KHÔNG chuyển màn hình
				// Ví dụ: "Bạn đã thoát và bị trừ 10 điểm"
				// strcpy(app.message, pkt.data);
			}
			// NẾU ĐANG CHƠI (Hết trận bình thường hoặc đối thủ thoát):
			else
			{
				strcpy(app.message, pkt.data);
				app.state = STATE_GAME_OVER; // Chỉ chuyển cảnh khi đang chơi
			}
			break;
		case MSG_LEADERBOARD:
			// Server gửi nội dung text trong pkt.data
			strcpy(app.leaderboard, pkt.data);
			app.state = STATE_LEADERBOARD; // Chuyển sang màn hình xem điểm
			break;
		case MSG_CHAT:
			// Khi Server gửi thông báo "Đối thủ muốn tái đấu" hoặc "Đang đợi..."
			strcpy(app.message, pkt.data);
			break;
		default:
			break;
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
// --- 1. SETUP WINSOCK (BẮT BUỘC CHO WINDOWS) ---
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Loi", "WSAStartup failed", NULL);
		printf("WSAStartup failed.\n");
		return 1;
	}
#endif
	// 1. Khởi tạo App
	memset(&app, 0, sizeof(App));
	initSDL();

	if (TTF_Init() == -1)
		exit(1);
	app.font = TTF_OpenFont("arial.ttf", 24);
	if (!app.font)
	{
		printf("Loi: Khong tim thay font arial.ttf\n");
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Loi Font", "Khong tim thay file arial.ttf! Hay copy file nay de canh game.exe", NULL);
		exit(1);
	}

	// 2. Kết nối Server

	// app.socket = socket(AF_INET, SOCK_STREAM, 0);
	// struct sockaddr_in addr;
	// addr.sin_family = AF_INET;
	// addr.sin_port = htons(8888);
	// inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	// if (connect(app.socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	// {
	// 	printf("Khong ket noi duoc server\n");
	// 	return 1;
	// }
	char *server_ip = "0.tcp.ap.ngrok.io";
	int server_port = 19255;

	// Nhận tham số từ dòng lệnh: game.exe <HOST> <PORT>
	if (argc >= 2)
		server_ip = argv[1];
	if (argc >= 3)
		server_port = atoi(argv[2]);

	app.socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	struct hostent *host;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);

	// QUAN TRỌNG: Phân giải tên miền (Ngrok) thành IP
	host = gethostbyname(server_ip);
	if (host == NULL)
	{
		addr.sin_addr.s_addr = inet_addr(server_ip); // Fallback nếu là IP số
	}
	else
	{
		memcpy(&addr.sin_addr, host->h_addr, host->h_length);
	}

	printf("Dang ket noi toi: %s:%d ...\n", server_ip, server_port);
	if (connect(app.socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		char errMsg[256];
		sprintf(errMsg, "Khong ket noi duoc toi %s:%d\nHay kiem tra lai Ngrok!", server_ip, server_port);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Loi Ket Noi", errMsg, NULL);
		return 1;
		printf("Khong ket noi duoc server!\n");
		return 1;
	}
	printf("Ket noi thanh cong!\n");

	pthread_t tid;
	pthread_create(&tid, NULL, receive_thread, NULL);

	app.state = STATE_MENU;
	SDL_StartTextInput();

	// 3. Vòng lặp chính
	while (1)
	{
		prepareScene();
		doInput();

		// --- XỬ LÝ CLICK TẠI MENU ---
		if (app.state == STATE_MENU && app.mouseDown)
		{
			if (app.mouseX >= 350 && app.mouseX <= 550)
			{
				if (app.mouseY >= 200 && app.mouseY <= 250)
				{ // Login
					app.state = STATE_LOGIN_INPUT;
					memset(app.inputUsername, 0, 32);
					memset(app.inputPassword, 0, 32);
					app.inputFocus = 0;
					strcpy(app.message, "");
				}
				else if (app.mouseY >= 300 && app.mouseY <= 350)
				{ // Register
					app.state = STATE_REGISTER_INPUT;
					memset(app.inputUsername, 0, 32);
					memset(app.inputPassword, 0, 32);
					memset(app.inputName, 0, 32);
					app.inputFocus = 0;
					strcpy(app.message, "");
				}
				else if (app.mouseY >= 400 && app.mouseY <= 450)
				{ // Exit
					exit(0);
				}
			}
			app.mouseDown = 0;
		}

		// --- XỬ LÝ CLICK TẠI LOGIN / REGISTER ---
		if ((app.state == STATE_LOGIN_INPUT || app.state == STATE_REGISTER_INPUT) && app.mouseDown)
		{
			int btnY = (app.state == STATE_REGISTER_INPUT) ? 400 : 320;

			// Nút Submit
			if (app.mouseX >= 350 && app.mouseX <= 550 && app.mouseY >= btnY && app.mouseY <= btnY + 40)
			{
				Packet pkt;
				if (app.state == STATE_LOGIN_INPUT)
				{
					printf("DEBUG: Da click nut Submit! Dang gui goi tin...\n"); // <--- THÊM DÒNG NÀY
					pkt.type = MSG_LOGIN;
					sprintf(pkt.data, "%s %s", app.inputUsername, app.inputPassword);
				}
				else
				{
					pkt.type = MSG_REGISTER;
					sprintf(pkt.data, "%s %s %s", app.inputUsername, app.inputPassword, app.inputName);
				}
				send(app.socket, (char *)&pkt, sizeof(Packet), 0);
			}

			// Chọn ô nhập liệu (Focus)
			if (app.mouseX >= 300 && app.mouseX <= 600)
			{
				if (app.mouseY >= 150 && app.mouseY <= 190)
					app.inputFocus = 0; // User
				else if (app.mouseY >= 230 && app.mouseY <= 270)
					app.inputFocus = 1; // Pass
				else if (app.state == STATE_REGISTER_INPUT && app.mouseY >= 310 && app.mouseY <= 350)
					app.inputFocus = 2; // Name
			}
			app.mouseDown = 0;
		}

		// --- XỬ LÝ CLICK TẠI LOBBY ---
		if (app.state == STATE_LOBBY && app.mouseDown)
		{
			// Nút Tìm trận
			if (app.mouseX >= 350 && app.mouseX <= 550 && app.mouseY >= 200 && app.mouseY <= 250)
			{
				Packet req;
				req.type = MSG_PLAY_REQ;
				send(app.socket, (char *)&req, sizeof(Packet), 0);
				app.state = STATE_WAITING;
				strcpy(app.message, "Dang tim doi thu...");
			}
			else if (app.mouseX >= 350 && app.mouseX <= 550 && app.mouseY >= 300 && app.mouseY <= 350)
			{
				Packet req;
				req.type = MSG_LEADERBOARD; // Gửi yêu cầu xem BXH
				send(app.socket, (char *)&req, sizeof(Packet), 0);

				// Server sẽ phản hồi MSG_LEADERBOARD -> receive_thread sẽ chuyển state
				strcpy(app.leaderboard, "Dang tai du lieu...");
			}
			// Nút Thoát
			else if (app.mouseX >= 350 && app.mouseX <= 550 && app.mouseY >= 400 && app.mouseY <= 450)
			{
				app.state = STATE_MENU;
				Packet pkt;
				pkt.type = MSG_LOGOUT;
				send(app.socket, (char *)&pkt, sizeof(Packet), 0);

				app.state = STATE_MENU;
				// Reset tên để lần sau nhập mới không bị dính tên cũ
				memset(app.inputUsername, 0, 32);
				memset(app.inputPassword, 0, 32);
			}
			app.mouseDown = 0;
		}
		if (app.state == STATE_LEADERBOARD && app.mouseDown)
		{
			// Nút Quay lại (Y: 500-550)
			if (app.mouseX >= 350 && app.mouseX <= 550 && app.mouseY >= 500 && app.mouseY <= 550)
			{
				app.state = STATE_LOBBY;
			}
			app.mouseDown = 0;
		}

		// --- XỬ LÝ CLICK TẠI WAITING ---
		if (app.state == STATE_WAITING && app.mouseDown)
		{
			// Nút Hủy tìm
			if (app.mouseX >= 350 && app.mouseX <= 550 && app.mouseY >= 350 && app.mouseY <= 400)
			{
				Packet pkt;
				pkt.type = MSG_CANCEL_FIND;
				send(app.socket, (char *)&pkt, sizeof(Packet), 0);
				app.state = STATE_LOBBY;
				strcpy(app.message, "");
			}
			app.mouseDown = 0;
		}

		// --- XỬ LÝ CLICK TẠI GAME ---
		// --- XỬ LÝ CLICK TẠI GAME ---
		if (app.state == STATE_GAME && app.mouseDown)
		{
			if (app.turn == app.player_id)
			{
				int c = app.mouseX / CELL_SIZE; // Cột
				int r = app.mouseY / CELL_SIZE; // Dòng

				// Kiểm tra ô trống
				if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && app.board[r][c] == 0)
				{
					Packet p;
					p.type = MSG_MOVE;
					p.x = r;
					p.y = c;
					// p.score = ... (Không cần gửi score khi move)
					send(app.socket, (char *)&p, sizeof(Packet), 0);

					// --- THÊM 2 DÒNG NÀY ---
					app.board[r][c] = app.player_id;		 // 1. Tự điền X/O của mình ngay lập tức
					app.turn = (app.player_id == 1) ? 2 : 1; // 2. Tự chuyển lượt (khóa chuột lại)
															 // -----------------------
				}
			}
			app.mouseDown = 0; // Reset chuột
		}

		// --- XỬ LÝ CLICK TẠI GAME OVER ---
		if (app.state == STATE_GAME_OVER && app.mouseDown)
		{
			int rightPanelX = BOARD_SIZE * CELL_SIZE + 20;

			// Nút "VE SANH" vẽ ở y=200, cao 40
			if (app.mouseX >= rightPanelX && app.mouseX <= rightPanelX + 160 &&
				app.mouseY >= 200 && app.mouseY <= 240) // <--- Sửa tọa độ Y
			{
				app.state = STATE_LOBBY;
				strcpy(app.message, "");
			}
			app.mouseDown = 0;
		}

		// Phím ESC để quay lại
		const Uint8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_ESCAPE])
		{
			if (app.state == STATE_LOGIN_INPUT || app.state == STATE_REGISTER_INPUT)
				app.state = STATE_MENU;
		}

		presentScene();
		SDL_Delay(16);
	}
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}