#include "input.h"
#include "common.h"
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
			break;

		default:
			break;
		}
	}
}