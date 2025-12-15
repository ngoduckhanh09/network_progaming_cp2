#ifndef PROTOCOL_H
#define PROTOCOL_H

// --- 1. CHỐNG LỆCH BYTE (RẤT QUAN TRỌNG) ---
// Giúp struct có kích thước giống hệt nhau trên mọi máy (Windows/Mac/Linux)
#pragma pack(1)

// --- 2. CÁC LOẠI TIN NHẮN (MESSAGE TYPES) ---
typedef enum
{
    MSG_LOGIN,    // Đăng nhập
    MSG_REGISTER, // Đăng ký
    MSG_WAIT,     // Thông báo: "Đang tìm đối thủ..."
    MSG_START,    // Bắt đầu game (kèm thông báo bạn là X hay O)
    MSG_MOVE,     // Gửi tọa độ nước đi (Row, Col)
    MSG_UPDATE,   // Cập nhật bàn cờ (Đối thủ vừa đánh vào ô này)
    MSG_CHAT,     // Chat
    MSG_QUIT,     // Thoát game
    MSG_END,
    MSG_LOGIN_SUCCESS, // <--- Mới
    MSG_LOGIN_FAIL,    // <--- Mới
    MSG_PLAY_REQ,      // <---       // Kết thúc (Thắng/Thua)
} MessageType;

// --- 3. CẤU TRÚC GÓI TIN (PACKET) ---
// Tổng kích thước gói này cố định (khoảng 270 bytes)
typedef struct
{
    MessageType type; // Loại tin nhắn (4 bytes)
    int player_id;    // ID người chơi (Để biết ai gửi)
    int x;            // Tọa độ dòng (Row)
    int y;            // Tọa độ cột (Col)
    char data[256];   // Dữ liệu text (Username, Chat, Thông báo...)
} Packet;

// Tắt chế độ nén byte
#pragma pack()

#endif // PROTOCOL_H