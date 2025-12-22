#ifndef AUTH_H
#define AUTH_H

// Kiểm tra user/pass. Trả về 1 nếu đúng, 0 nếu sai.
int check_login(const char *username, const char *password);

#endif // AUTH_H