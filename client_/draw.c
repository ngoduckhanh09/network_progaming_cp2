#include "common.h"
#include "draw.h"

extern App app;

// Màu sắc (Định nghĩa cho dễ sửa)
SDL_Color COL_BG = {245, 245, 220, 255};		// Màu kem (nền bàn cờ)
SDL_Color COL_GRID = {180, 180, 180, 255};		// Màu lưới xám nhạt
SDL_Color COL_X = {220, 20, 60, 255};			// Đỏ thẫm
SDL_Color COL_O = {30, 144, 255, 255};			// Xanh dương
SDL_Color COL_BTN_IDLE = {70, 130, 180, 255};	// Xanh thép
SDL_Color COL_BTN_HOVER = {100, 149, 237, 255}; // Xanh sáng hơn
SDL_Color COL_TEXT = {50, 50, 50, 255};			// Chữ đen xám

// --- HÀM CƠ BẢN: VẼ CHỮ ---
void drawText(char *text, int x, int y, int r, int g, int b)
{
	if (!text || !app.font)
		return;
	SDL_Color color = {r, g, b, 255};
	SDL_Surface *surface = TTF_RenderText_Solid(app.font, text, color);
	if (!surface)
		return;
	SDL_Texture *texture = SDL_CreateTextureFromSurface(app.renderer, surface);
	SDL_Rect destRect = {x, y, surface->w, surface->h};
	SDL_RenderCopy(app.renderer, texture, NULL, &destRect);
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}
// --- HÀM MỚI: Vẽ chữ tự động xuống dòng ---
void drawTextWrapped(char *text, int x, int y, int maxWidth, int r, int g, int b)
{
	if (!text || strlen(text) == 0 || !app.font)
		return;

	char buffer[1024];
	strcpy(buffer, text); // Copy chuỗi để xử lý

	SDL_Color color = {r, g, b, 255};
	int currentY = y;
	int spaceW, h;
	TTF_SizeText(app.font, " ", &spaceW, &h); // Lấy chiều cao dòng và độ rộng dấu cách

	char line[256] = "";			  // Bộ đệm cho dòng hiện tại
	char *word = strtok(buffer, " "); // Tách từng từ theo dấu cách

	while (word != NULL)
	{
		char tempLine[256];
		strcpy(tempLine, line);
		if (strlen(line) > 0)
			strcat(tempLine, " ");
		strcat(tempLine, word);

		int w;
		TTF_SizeText(app.font, tempLine, &w, NULL);

		// Nếu dòng mới dài quá maxWidth -> In dòng cũ và xuống dòng
		if (w > maxWidth)
		{
			drawText(line, x, currentY, r, g, b);
			currentY += h + 5;	// Xuống dòng (cộng thêm 5px giãn dòng)
			strcpy(line, word); // Bắt đầu dòng mới với từ hiện tại
		}
		else
		{
			strcpy(line, tempLine); // Vẫn đủ chỗ, gộp từ vào dòng
		}
		word = strtok(NULL, " ");
	}
	// In nốt dòng cuối cùng
	if (strlen(line) > 0)
	{
		drawText(line, x, currentY, r, g, b);
	}
}
// Hàm vẽ chữ căn giữa
void drawTextCentered(char *text, int centerX, int y, SDL_Color col)
{
	if (!text || !app.font)
		return;
	int w, h;
	TTF_SizeText(app.font, text, &w, &h);
	drawText(text, centerX - w / 2, y, col.r, col.g, col.b);
}

// --- HÀM VẼ NÚT ĐẸP HƠN ---
void drawButton(SDL_Rect rect, char *label, int mouseX, int mouseY)
{
	// Kiểm tra chuột
	int hover = (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
				 mouseY >= rect.y && mouseY <= rect.y + rect.h);

	// Vẽ bóng nút (tạo độ nổi)
	SDL_Rect shadow = {rect.x + 3, rect.y + 3, rect.w, rect.h};
	SDL_SetRenderDrawColor(app.renderer, 50, 50, 50, 100);
	SDL_RenderFillRect(app.renderer, &shadow);

	// Vẽ nền nút
	if (hover)
		SDL_SetRenderDrawColor(app.renderer, COL_BTN_HOVER.r, COL_BTN_HOVER.g, COL_BTN_HOVER.b, 255);
	else
		SDL_SetRenderDrawColor(app.renderer, COL_BTN_IDLE.r, COL_BTN_IDLE.g, COL_BTN_IDLE.b, 255);
	SDL_RenderFillRect(app.renderer, &rect);

	// Vẽ viền nút
	SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(app.renderer, &rect);

	// Căn giữa chữ
	if (label && app.font)
	{
		int w, h;
		TTF_SizeText(app.font, label, &w, &h);
		drawText(label, rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, 255, 255, 255);
	}
}

// --- VẼ MENU ---
void drawMainMenu(void)
{
	drawTextCentered("GAME CARO ONLINE", 450, 100, COL_X); // Tiêu đề đỏ

	SDL_Rect btnLogin = {350, 200, 200, 50};
	SDL_Rect btnReg = {350, 300, 200, 50};
	SDL_Rect btnExit = {350, 400, 200, 50};

	drawButton(btnLogin, "DANG NHAP", app.mouseX, app.mouseY);
	drawButton(btnReg, "DANG KY", app.mouseX, app.mouseY);
	drawButton(btnExit, "THOAT", app.mouseX, app.mouseY);
}

// --- VẼ MÀN HÌNH LOGIN/REGISTER ---
// Hàm phụ: Vẽ 1 ô Input
void drawInputBox(int x, int y, int w, int h, char *label, char *text, int isFocus, int isPassword)
{
	// Vẽ nhãn (Label)
	drawText(label, x, y - 25, 200, 200, 200);

	// Vẽ khung
	SDL_Rect rect = {x, y, w, h};
	if (isFocus)
		SDL_SetRenderDrawColor(app.renderer, 0, 255, 0, 255); // Viền xanh lá khi focus
	else
		SDL_SetRenderDrawColor(app.renderer, 100, 100, 100, 255); // Viền xám thường
	SDL_RenderDrawRect(app.renderer, &rect);

	// Vẽ nội dung bên trong
	if (text && strlen(text) > 0)
	{
		if (isPassword)
		{
			// Thay thế bằng dấu *
			char mask[32];
			memset(mask, '*', strlen(text));
			mask[strlen(text)] = '\0';
			drawText(mask, x + 10, y + 10, 255, 255, 255);
		}
		else
		{
			drawText(text, x + 10, y + 10, 255, 255, 255);
		}
	}
}

void drawAuthScreen(int isRegister)
{
	// Tiêu đề
	char *title = isRegister ? "DANG KY TAI KHOAN" : "DANG NHAP";
	drawTextCentered(title, 450, 50, (SDL_Color){255, 255, 0, 255});

	// 1. Ô Username (Focus = 0)
	drawInputBox(300, 150, 300, 40, "Ten dang nhap:", app.inputUsername, (app.inputFocus == 0), 0);

	// 2. Ô Password (Focus = 1)
	drawInputBox(300, 230, 300, 40, "Mat khau:", app.inputPassword, (app.inputFocus == 1), 1);

	int btnY = 320;

	// 3. Ô Player Name (Chỉ hiện khi Đăng ký, Focus = 2)
	if (isRegister)
	{
		drawInputBox(300, 310, 300, 40, "Ten trong game:", app.inputName, (app.inputFocus == 2), 0);
		btnY = 400; // Đẩy nút xuống thấp hơn
	}

	// Nút Action
	SDL_Rect btn = {350, btnY, 200, 40};
	char *btnLabel = isRegister ? "DANG KY NGAY" : "VAO GAME";
	drawButton(btn, btnLabel, app.mouseX, app.mouseY);

	// Hướng dẫn
	drawTextCentered("(Dung TAB de chuyen o nhap)", 450, btnY + 50, (SDL_Color){150, 150, 150, 255});
	drawText("<< ESC: Quay lai", 50, 550, 100, 100, 100);

	// Thông báo lỗi
	if (strlen(app.message) > 0)
		drawTextCentered(app.message, 450, btnY + 90, (SDL_Color){255, 100, 100, 255});
}
// --- VẼ LOBBY ---
void drawLobby(void)
{
	drawTextCentered("SANH CHO (LOBBY)", 450, 100, (SDL_Color){255, 215, 0, 255}); // Vàng kim

	SDL_Rect btnFind = {350, 200, 200, 50};
	SDL_Rect btnScore = {350, 300, 200, 50};
	SDL_Rect btnBack = {350, 400, 200, 50};

	drawButton(btnFind, "TIM TRAN", app.mouseX, app.mouseY);
	drawButton(btnScore, "DIEM CAO", app.mouseX, app.mouseY);
	drawButton(btnBack, "DANG XUAT", app.mouseX, app.mouseY);

	if (strlen(app.message) > 0)
		drawTextCentered(app.message, 450, 500, (SDL_Color){0, 255, 0, 255});
}

// --- VẼ BÀN CỜ (Đã cải tiến) ---
void drawBoard(void)
{
	// 1. Vẽ nền bàn cờ
	SDL_SetRenderDrawColor(app.renderer, COL_BG.r, COL_BG.g, COL_BG.b, 255);
	SDL_Rect boardRect = {0, 0, BOARD_SIZE * CELL_SIZE, BOARD_SIZE * CELL_SIZE};
	SDL_RenderFillRect(app.renderer, &boardRect);

	// 2. Vẽ lưới
	SDL_SetRenderDrawColor(app.renderer, COL_GRID.r, COL_GRID.g, COL_GRID.b, 255);
	for (int i = 0; i <= BOARD_SIZE; i++)
	{
		SDL_RenderDrawLine(app.renderer, 0, i * CELL_SIZE, BOARD_SIZE * CELL_SIZE, i * CELL_SIZE);
		SDL_RenderDrawLine(app.renderer, i * CELL_SIZE, 0, i * CELL_SIZE, BOARD_SIZE * CELL_SIZE);
	}

	// 3. Vẽ quân cờ (Căn giữa ô)
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			if (app.board[i][j] == 1) // X
				drawText("X", j * CELL_SIZE + 7, i * CELL_SIZE + 2, COL_X.r, COL_X.g, COL_X.b);
			else if (app.board[i][j] == 2) // O
				drawText("O", j * CELL_SIZE + 7, i * CELL_SIZE + 2, COL_O.r, COL_O.g, COL_O.b);
		}
	}
}
// --- HÀM VẼ THÔNG TIN NGƯỜI DÙNG (CẬP NHẬT) ---
void drawUserInfo(void)
{
	if (app.state == STATE_MENU || app.state == STATE_LOGIN_INPUT || app.state == STATE_REGISTER_INPUT)
		return;

	char buffer[64];
	sprintf(buffer, "User: %s | Diem: %d", app.inputUsername, app.score); // <--- HIỆN ĐIỂM

	if (app.state == STATE_LOBBY || app.state == STATE_WAITING)
	{
		drawText(buffer, 10, 10, 255, 255, 0);
	}
	else if (app.state == STATE_GAME || app.state == STATE_GAME_OVER)
	{
		int rightPanelX = BOARD_SIZE * CELL_SIZE + 20;
		drawText(buffer, rightPanelX, 550, 255, 255, 0);
	}
}
void drawLeaderboard(void)
{
	drawTextCentered("BANG XEP HANG (TOP 10)", 450, 50, (SDL_Color){255, 215, 0, 255});

	// Vẽ nội dung BXH (Server gửi về dạng chuỗi nhiều dòng)
	// SDL_ttf không hỗ trợ xuống dòng tự động (\n), ta cần tách dòng thủ công
	int y = 120;
	char temp[1024];
	strcpy(temp, app.leaderboard);

	char *line = strtok(temp, "\n");
	while (line != NULL)
	{
		drawTextCentered(line, 450, y, (SDL_Color){255, 255, 255, 255});
		y += 40; // Xuống dòng
		line = strtok(NULL, "\n");
	}

	// Nút Quay lại
	SDL_Rect btnBack = {350, 500, 200, 50};
	drawButton(btnBack, "QUAY LAI", app.mouseX, app.mouseY);
}
// --- PREPARE SCENE ---
// --- PREPARE SCENE ---
void prepareScene(void)
{
	SDL_SetRenderDrawColor(app.renderer, 40, 44, 52, 255);
	SDL_RenderClear(app.renderer);

	switch (app.state)
	{
	case STATE_MENU:
		drawMainMenu();
		break;
	case STATE_LOGIN_INPUT:
		drawAuthScreen(0);
		break;
	case STATE_REGISTER_INPUT:
		drawAuthScreen(1);
		break;
	case STATE_LOBBY:
		drawLobby();
		drawUserInfo(); // <--- THÊM VÀO ĐÂY
		break;
	case STATE_GAME:
	case STATE_GAME_OVER:
		drawBoard();
		drawUserInfo(); // <--- THÊM VÀO ĐÂY

		// --- Panel thông tin bên phải ---
		int rightPanelX = BOARD_SIZE * CELL_SIZE + 20;

		if (app.state == STATE_GAME_OVER)
		{
			drawText("KET THUC!", rightPanelX, 50, 255, 0, 0);
			int maxW = 260; // Chiều rộng tối đa cho dòng chữ (ước lượng)
			drawTextWrapped(app.message, rightPanelX, 100, maxW, 255, 255, 255);
			SDL_Rect btnExit = {rightPanelX, 200, 160, 40};
			drawButton(btnExit, "VE SANH", app.mouseX, app.mouseY);
		}
		else
		{
			char buf[64];
			sprintf(buf, "Ban la: %s", (app.player_id == 1) ? "X (Do)" : "O (Xanh)");
			drawText(buf, rightPanelX, 30, 255, 255, 255);

			if (app.turn == app.player_id)
				drawText("-> LUOT CUA BAN", rightPanelX, 80, 0, 255, 0);
			else
				drawText("-> Doi doi thu...", rightPanelX, 80, 200, 200, 200);
		}
		break;

	case STATE_WAITING:
		drawTextCentered("DANG TIM DOI THU...", 450, 250, (SDL_Color){0, 255, 255, 255});
		SDL_Rect btnCancel = {350, 350, 200, 50};
		drawButton(btnCancel, "HUY TIM", app.mouseX, app.mouseY);
		drawUserInfo(); // <--- THÊM VÀO ĐÂY
		break;
	case STATE_LEADERBOARD: // <--- THÊM CASE NÀY
		drawLeaderboard();
		drawUserInfo();
		break;
	}
}
void presentScene(void)
{
	SDL_RenderPresent(app.renderer);
}