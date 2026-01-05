#ifndef PROTOCOL_H
#define PROTOCOL_H

//  CHỐNG LỆCH BYTE

#pragma pack(1)

// ---  (MESSAGE TYPES) ---
typedef enum
{
    MSG_LOGIN,    // Đăng nhập
    MSG_REGISTER, // Đăng ký
    MSG_WAIT,     // Thông báo: "Đang tìm đối thủ..."
    MSG_START,    // Bắt đầu game
    MSG_MOVE,     // Gửi tọa độ nước đi
    MSG_UPDATE,   // Cập nhật bàn cờ
    MSG_CHAT,     // Chat
    MSG_QUIT,     // Thoát game
    MSG_END,
    MSG_LOGIN_SUCCESS, //
    MSG_LOGIN_FAIL,    //
    MSG_PLAY_REQ,      // Kết thúc (Thắng/Thua)
    MSG_PLAY_AGAIN,
    MSG_LEADERBOARD,
    MSG_CANCEL_FIND,
} MessageType;

//  CẤU TRÚC GÓI TIN
typedef struct
{
    MessageType type; // Loại tin nhắn (4 bytes)
    int player_id;    // ID người chơi (Để biết ai gửi)
    int x;            // Tọa độ dòng (Row)
    int y;            // Tọa độ cột (Col)
    int score;        // <--- THÊM BIẾN NÀY (Lưu điểm số/Elo)
    char data[256];   // Dữ liệu text (Username, Chat, Thông báo...)
} Packet;

#pragma pack()

#endif // PROTOCOL_H