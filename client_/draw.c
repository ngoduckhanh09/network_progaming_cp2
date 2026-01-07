#include "common.h"
#include "draw.h"

extern App app;

// Màu sắc
SDL_Color COL_BG = {245, 245, 220, 255};		// Màu kem
SDL_Color COL_GRID = {180, 180, 180, 255};		// Màu lưới
SDL_Color COL_X = {220, 20, 60, 255};			// Đỏ
SDL_Color COL_O = {30, 144, 255, 255};			// Xanh
SDL_Color COL_BTN_IDLE = {70, 130, 180, 255};	// Xanh thép
SDL_Color COL_BTN_HOVER = {100, 149, 237, 255}; // Xanh sáng
SDL_Color COL_TEXT = {50, 50, 50, 255};			// Chữ đen

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
	strcpy(buffer, text);

	SDL_Color color = {r, g, b, 255};
	int currentY = y;
	int spaceW, h;
	TTF_SizeText(app.font, " ", &spaceW, &h);

	char line[256] = "";
	char *word = strtok(buffer, " ");

	while (word != NULL)
	{
		char tempLine[256];
		strcpy(tempLine, line);
		if (strlen(line) > 0)
			strcat(tempLine, " ");
		strcat(tempLine, word);

		int w;
		TTF_SizeText(app.font, tempLine, &w, NULL);

		if (w > maxWidth)
		{
			drawText(line, x, currentY, r, g, b);
			currentY += h + 5;
			strcpy(line, word);
		}
		else
		{
			strcpy(line, tempLine);
		}
		word = strtok(NULL, " ");
	}
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

// --- HÀM VẼ NÚT ---
void drawButton(SDL_Rect rect, char *label, int mouseX, int mouseY)
{
	int hover = (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
				 mouseY >= rect.y && mouseY <= rect.y + rect.h);

	SDL_Rect shadow = {rect.x + 3, rect.y + 3, rect.w, rect.h};
	SDL_SetRenderDrawColor(app.renderer, 50, 50, 50, 100);
	SDL_RenderFillRect(app.renderer, &shadow);

	if (hover)
		SDL_SetRenderDrawColor(app.renderer, COL_BTN_HOVER.r, COL_BTN_HOVER.g, COL_BTN_HOVER.b, 255);
	else
		SDL_SetRenderDrawColor(app.renderer, COL_BTN_IDLE.r, COL_BTN_IDLE.g, COL_BTN_IDLE.b, 255);
	SDL_RenderFillRect(app.renderer, &rect);

	SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(app.renderer, &rect);

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
	drawTextCentered("GAME CARO ONLINE", 450, 100, COL_X);
	SDL_Rect btnLogin = {350, 200, 200, 50};
	SDL_Rect btnReg = {350, 300, 200, 50};
	SDL_Rect btnExit = {350, 400, 200, 50};
	drawButton(btnLogin, "DANG NHAP", app.mouseX, app.mouseY);
	drawButton(btnReg, "DANG KY", app.mouseX, app.mouseY);
	drawButton(btnExit, "THOAT", app.mouseX, app.mouseY);
}

// --- VẼ INPUT ---
void drawInputBox(int x, int y, int w, int h, char *label, char *text, int isFocus, int isPassword)
{
	drawText(label, x, y - 25, 200, 200, 200);
	SDL_Rect rect = {x, y, w, h};
	if (isFocus)
		SDL_SetRenderDrawColor(app.renderer, 0, 255, 0, 255);
	else
		SDL_SetRenderDrawColor(app.renderer, 100, 100, 100, 255);
	SDL_RenderDrawRect(app.renderer, &rect);

	if (text && strlen(text) > 0)
	{
		if (isPassword)
		{
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
	char *title = isRegister ? "DANG KY TAI KHOAN" : "DANG NHAP";
	drawTextCentered(title, 450, 50, (SDL_Color){255, 255, 0, 255});

	drawInputBox(300, 150, 300, 40, "Ten dang nhap:", app.inputUsername, (app.inputFocus == 0), 0);
	drawInputBox(300, 230, 300, 40, "Mat khau:", app.inputPassword, (app.inputFocus == 1), 1);

	int btnY = 320;
	if (isRegister)
	{
		drawInputBox(300, 310, 300, 40, "Ten trong game:", app.inputName, (app.inputFocus == 2), 0);
		btnY = 400;
	}

	SDL_Rect btn = {350, btnY, 200, 40};
	char *btnLabel = isRegister ? "DANG KY NGAY" : "VAO GAME";
	drawButton(btn, btnLabel, app.mouseX, app.mouseY);

	drawTextCentered("(Dung TAB de chuyen o nhap)", 450, btnY + 50, (SDL_Color){150, 150, 150, 255});
	drawText("<< ESC: Quay lai", 50, 550, 100, 100, 100);

	if (strlen(app.message) > 0)
		drawTextCentered(app.message, 450, btnY + 90, (SDL_Color){255, 100, 100, 255});
}

// --- VẼ LOBBY ---
void drawLobby(void)
{
	drawTextCentered("", 450, 100, (SDL_Color){255, 215, 0, 255});
	SDL_Rect btnFind = {350, 200, 200, 50};
	SDL_Rect btnScore = {350, 300, 200, 50};
	SDL_Rect btnBack = {350, 400, 200, 50};

	drawButton(btnFind, "TIM TRAN", app.mouseX, app.mouseY);
	drawButton(btnScore, "DIEM CAO", app.mouseX, app.mouseY);
	drawButton(btnBack, "DANG XUAT", app.mouseX, app.mouseY);

	if (strlen(app.message) > 0)
		drawTextCentered(app.message, 450, 500, (SDL_Color){0, 255, 0, 255});
}

// --- VẼ BÀN CỜ ---
void drawBoard(void)
{
	SDL_SetRenderDrawColor(app.renderer, COL_BG.r, COL_BG.g, COL_BG.b, 255);
	SDL_Rect boardRect = {0, 0, BOARD_SIZE * CELL_SIZE, BOARD_SIZE * CELL_SIZE};
	SDL_RenderFillRect(app.renderer, &boardRect);

	SDL_SetRenderDrawColor(app.renderer, COL_GRID.r, COL_GRID.g, COL_GRID.b, 255);
	for (int i = 0; i <= BOARD_SIZE; i++)
	{
		SDL_RenderDrawLine(app.renderer, 0, i * CELL_SIZE, BOARD_SIZE * CELL_SIZE, i * CELL_SIZE);
		SDL_RenderDrawLine(app.renderer, i * CELL_SIZE, 0, i * CELL_SIZE, BOARD_SIZE * CELL_SIZE);
	}

	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			if (app.board[i][j] == 1)
				drawText("X", j * CELL_SIZE + 7, i * CELL_SIZE + 2, COL_X.r, COL_X.g, COL_X.b);
			else if (app.board[i][j] == 2)
				drawText("O", j * CELL_SIZE + 7, i * CELL_SIZE + 2, COL_O.r, COL_O.g, COL_O.b);
		}
	}
}

// --- SỬA: CHỈ VẼ Ở LOBBY/WAITING ---
void drawUserInfo(void)
{
	// Chỉ hiện ở Lobby và Waiting (Game sẽ có panel riêng)
	if (app.state == STATE_LOBBY || app.state == STATE_WAITING)
	{
		char buffer[64];
		sprintf(buffer, "User: %s | Diem: %d", app.inputUsername, app.score);
		drawText(buffer, 10, 10, 255, 255, 0);
	}
}

void drawLeaderboard(void)
{
	drawTextCentered("BANG XEP HANG (TOP 10)", 450, 50, (SDL_Color){255, 215, 0, 255});
	int y = 120;
	char temp[1024];
	strcpy(temp, app.leaderboard);
	char *line = strtok(temp, "\n");
	while (line != NULL)
	{
		drawTextCentered(line, 450, y, (SDL_Color){255, 255, 255, 255});
		y += 40;
		line = strtok(NULL, "\n");
	}
	SDL_Rect btnBack = {350, 500, 200, 50};
	drawButton(btnBack, "QUAY LAI", app.mouseX, app.mouseY);
}

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
		drawUserInfo();
		break;

	// --- PHẦN QUAN TRỌNG: CẬP NHẬT GIAO DIỆN GAME ---
	case STATE_GAME:
	case STATE_GAME_OVER:
		drawBoard();

		// --- VẼ PANEL THÔNG TIN BÊN PHẢI ---
		int rightPanelX = BOARD_SIZE * CELL_SIZE + 20;
		char buffer[100];

		// 1. THÔNG TIN CỦA BẠN
		SDL_Color colorMe = (app.player_id == 1) ? COL_X : COL_O; // Mình là X thì đỏ, O thì xanh
		drawText("--- BAN ---", rightPanelX, 30, colorMe.r, colorMe.g, colorMe.b);

		sprintf(buffer, "%s %s", app.inputUsername, (app.player_id == 1) ? "(X)" : "(O)");
		drawText(buffer, rightPanelX, 60, 255, 255, 255);

		sprintf(buffer, "Diem: %d", app.score);
		drawText(buffer, rightPanelX, 90, 255, 255, 255);

		// 2. THÔNG TIN ĐỐI THỦ (Cách ra một chút)
		SDL_Color colorOpp = (app.player_id == 1) ? COL_O : COL_X; // Ngược lại với mình
		drawText("--- DOI THU ---", rightPanelX, 150, colorOpp.r, colorOpp.g, colorOpp.b);

		sprintf(buffer, "%s %s", app.opponentName, (app.player_id == 1) ? "(O)" : "(X)");
		drawText(buffer, rightPanelX, 180, 255, 255, 255);

		sprintf(buffer, "Diem: %d", app.opponentScore);
		drawText(buffer, rightPanelX, 210, 255, 255, 255);

		// 3. TRẠNG THÁI / KẾT QUẢ
		int statusY = 300; // Vị trí Y bắt đầu vẽ trạng thái

		if (app.state == STATE_GAME_OVER)
		{
			drawText("KET THUC!", rightPanelX, statusY, 255, 0, 0);
			drawTextWrapped(app.message, rightPanelX, statusY + 40, 260, 255, 255, 255);

			// Nút điều khiển
			SDL_Rect btnExit = {rightPanelX, 450, 160, 40};
			drawButton(btnExit, "VE SANH", app.mouseX, app.mouseY);

			SDL_Rect btnRematch = {rightPanelX, 500, 160, 40};
			drawButton(btnRematch, "TAI DAU", app.mouseX, app.mouseY);
		}
		else // ĐANG CHƠI
		{
			if (app.turn == app.player_id)
				drawText("-> LUOT CUA BAN", rightPanelX, statusY, 0, 255, 0);
			else
				drawText("-> Doi doi thu...", rightPanelX, statusY, 200, 200, 200);

			// Nút chat/hàng nếu muốn thêm sau này...
			SDL_Rect btnQuit = {rightPanelX, 500, 160, 40};
			// Vẽ nút với nhãn "THOAT TRAN" (hoặc "DAU HANG")
			drawButton(btnQuit, "THOAT TRAN", app.mouseX, app.mouseY);
		}
		break;
		// ----------------------------------------------------

	case STATE_WAITING:
		drawTextCentered("DANG TIM DOI THU...", 450, 250, (SDL_Color){0, 255, 255, 255});
		SDL_Rect btnCancel = {350, 350, 200, 50};
		drawButton(btnCancel, "HUY TIM", app.mouseX, app.mouseY);
		drawUserInfo();
		break;
	case STATE_LEADERBOARD:
		drawLeaderboard();
		drawUserInfo();
		break;
	}
}

void presentScene(void)
{
	SDL_RenderPresent(app.renderer);
}