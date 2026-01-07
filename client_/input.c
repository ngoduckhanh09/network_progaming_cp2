#include "input.h"
#include "common.h"
#include "protocol.h"
extern App app;

void doInput(void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			exit(0);
			break;

		case SDL_TEXTINPUT:
			// Chỉ xử lý nhập liệu ở màn hình Login/Register
			if (app.state == STATE_LOGIN_INPUT || app.state == STATE_REGISTER_INPUT)
			{
				char *target = NULL;
				// Xác định đang nhập vào ô nào
				if (app.inputFocus == 0)
					target = app.inputUsername;
				else if (app.inputFocus == 1)
					target = app.inputPassword;
				else if (app.inputFocus == 2)
					target = app.inputName;

				if (target && strlen(target) < 30) // Giới hạn 30 ký tự
				{
					strcat(target, event.text.text);
				}
			}
			break;

		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_BACKSPACE)
			{
				// Xóa ký tự
				char *target = NULL;
				if (app.state == STATE_LOGIN_INPUT || app.state == STATE_REGISTER_INPUT)
				{
					if (app.inputFocus == 0)
						target = app.inputUsername;
					else if (app.inputFocus == 1)
						target = app.inputPassword;
					else if (app.inputFocus == 2)
						target = app.inputName;
				}

				if (target && strlen(target) > 0)
				{
					target[strlen(target) - 1] = '\0';
				}
			}

			// Phím TAB: Chuyển ô nhập
			if (event.key.keysym.sym == SDLK_TAB)
			{
				int maxFields = (app.state == STATE_LOGIN_INPUT) ? 1 : 2; // Login có 0,1. Reg có 0,1,2
				app.inputFocus++;
				if (app.inputFocus > maxFields)
					app.inputFocus = 0;
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			app.mouseDown = 1;
			app.mouseX = event.button.x;
			app.mouseY = event.button.y;
			// --- 1. XỬ LÝ KHI ĐANG CHƠI (THÊM ĐOẠN NÀY) ---
			if (app.state == STATE_GAME)
			{
				int rightPanelX = BOARD_SIZE * CELL_SIZE + 20;

				// A. Kiểm tra nút "THOÁT TRẬN" (Vị trí phải khớp với bên draw.c)
				SDL_Rect btnQuit = {rightPanelX, 500, 160, 40};
				if (app.mouseX >= btnQuit.x && app.mouseX <= btnQuit.x + btnQuit.w &&
					app.mouseY >= btnQuit.y && app.mouseY <= btnQuit.y + btnQuit.h)
				{
					// Gửi thông báo rời phòng lên Server
					Packet pkt;
					memset(&pkt, 0, sizeof(Packet));
					pkt.type = MSG_LEAVE_ROOM;
					pkt.player_id = app.player_id;
					send(app.socket, (char *)&pkt, sizeof(Packet), 0);

					// Client tự xử lý thua và về sảnh
					app.state = STATE_LOBBY;
					strcpy(app.message, "");
					memset(app.board, 0, sizeof(app.board)); // Xóa bàn cờ cũ
				}

				// B. Kiểm tra đánh cờ (Click vào bàn cờ)
				// Nếu không có đoạn này bạn sẽ không đánh dấu X/O được!
				else if (app.mouseX < BOARD_SIZE * CELL_SIZE && app.mouseY < BOARD_SIZE * CELL_SIZE)
				{
					if (app.turn == app.player_id) // Chỉ đánh khi đến lượt
					{
						int c = app.mouseX / CELL_SIZE;
						int r = app.mouseY / CELL_SIZE;

						if (app.board[r][c] == 0) // Chỉ đánh vào ô trống
						{
							Packet pkt;
							memset(&pkt, 0, sizeof(Packet));
							pkt.type = MSG_MOVE;
							pkt.x = r;
							pkt.y = c;
							pkt.player_id = app.player_id;
							send(app.socket, (char *)&pkt, sizeof(Packet), 0);

							// Cập nhật ngay phía client để không bị lag
							app.board[r][c] = app.player_id;
							app.turn = 0; // Hết lượt, chờ đối thủ
						}
					}
				}
			}
			// XỬ LÝ CLICK Ở MÀN HÌNH GAME OVER
			if (app.state == STATE_GAME_OVER)
			{
				int rightPanelX = BOARD_SIZE * CELL_SIZE + 20;

				// 1. Check nút "VỀ SẢNH" (Giữ nguyên logic cũ nếu có)
				SDL_Rect btnExit = {rightPanelX, 450, 160, 40};
				if (app.mouseX >= btnExit.x && app.mouseX <= btnExit.x + btnExit.w &&
					app.mouseY >= btnExit.y && app.mouseY <= btnExit.y + btnExit.h)
				{
					// Gửi MSG_QUIT hoặc chuyển state về Lobby (code cũ của bạn)
					// 1. Gửi thông báo rời phòng lên Server
					Packet pkt;
					memset(&pkt, 0, sizeof(Packet));
					pkt.type = MSG_LEAVE_ROOM;
					pkt.player_id = app.player_id;
					send(app.socket, (char *)&pkt, sizeof(Packet), 0);

					// 2. Xóa thông báo cũ ("Bạn thắng +10d"...)
					strcpy(app.message, "");

					// 3. Chuyển về sảnh
					app.state = STATE_LOBBY;

					// ----------------
					// Lưu ý: Nếu server cần biết client thoát trận, hãy gửi MSG_CANCEL_FIND hoặc MSG_QUIT
					app.state = STATE_LOBBY;
				}

				// 2. Check nút "TÁI ĐẤU" (MỚI)
				SDL_Rect btnRematch = {rightPanelX, 500, 160, 40};
				if (app.mouseX >= btnRematch.x && app.mouseX <= btnRematch.x + btnRematch.w &&
					app.mouseY >= btnRematch.y && app.mouseY <= btnRematch.y + btnRematch.h)
				{
					// Gửi yêu cầu tái đấu
					Packet pkt;
					memset(&pkt, 0, sizeof(Packet));
					pkt.type = MSG_PLAY_AGAIN;
					pkt.player_id = app.player_id;
					send(app.socket, (char *)&pkt, sizeof(Packet), 0);

					// Cập nhật thông báo cho người dùng biết là đã bấm
					strcpy(app.message, "Dang cho doi thu...");
				}
			}
			break;

		default:
			break;
		}
	}
}