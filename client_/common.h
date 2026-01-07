#ifndef COMMON_H
#define COMMON_H

/* * Copyright (C) 2015-2018,2022 Parallel Realities. All rights reserved.
 */

// 1. Thư viện chuẩn (Dùng < >)
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 2. Thư viện SDL (Dùng < >)
#include <SDL.h>
#include <SDL_ttf.h>

// 3. File nội bộ (Dùng " ")
#include "defs.h"
#include "structs.h"

// 4. Xử lý đa nền tảng (Windows/Linux)
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
// #pragma comment(lib, "ws2_32.lib") // Không cần dòng này trên MinGW (đã có -lws2_32)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#endif // Kết thúc COMMON_H