#ifndef AUTH_H
#define AUTH_H

int check_login(char *username, char *password);
int register_user(char *username, char *password, char *name);
void update_game_result(char *winner, char *loser); // Sửa hàm này
int get_user_score(char *username);                 // Hàm mới lấy điểm
void get_leaderboard(char *buffer);                 // Hàm mới lấy BXH
#endif