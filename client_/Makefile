# Trình biên dịch
CC = gcc

# Các cờ cho trình biên dịch (Compiler Flags)
# -Wall: Hiển thị tất cả cảnh báo
# -g: Thêm thông tin debug (để dùng gdb nếu cần)
# $(shell sdl2-config --cflags): Tự động lấy các include path của SDL2
CFLAGS = -Wall -g $(shell sdl2-config --cflags)

# Các cờ cho trình liên kết (Linker Flags)
# $(shell sdl2-config --libs): Tự động lấy các thư viện SDL2 cần link
# -lm: Link thư viện toán học (math.h)
LDFLAGS = $(shell sdl2-config --libs) -lm -lSDL2_ttf

# Danh sách các file nguồn
SRCS = main.c input.c init.c draw.c

# Tạo danh sách file object (.o) tương ứng từ file nguồn
OBJS = $(SRCS:.c=.o)

# Tên file thực thi đầu ra
EXEC = game

# Mục tiêu mặc định (khi gõ 'make')
all: $(EXEC)

# Quy tắc để liên kết (link) các file object thành file thực thi
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

# Quy tắc để biên dịch từng file .c thành file .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Quy tắc dọn dẹp (xóa file object và file thực thi cũ)
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean