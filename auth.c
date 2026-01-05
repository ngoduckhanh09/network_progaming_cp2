#include "auth.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ACCOUNT_FILE "accounts.txt"

// Hàm kiểm tra đăng nhập
int check_login(char *username, char *password)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f)
        return 0;

    char u[32], p[32], n[32];
    int s; // Biến tạm để hứng điểm số

    // SỬA: Luôn đọc 4 trường: user pass name score
    while (fscanf(f, "%s %s %s %d", u, p, n, &s) != EOF)
    {
        if (strcmp(u, username) == 0)
        {
            if (strcmp(p, password) == 0)
            {
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

// Hàm đăng ký
int register_user(char *username, char *password, char *name)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (f)
    {
        char u[32], p[32], n[32];
        int s;
        // SỬA: Đọc 4 trường để tránh lệch dòng
        while (fscanf(f, "%s %s %s %d", u, p, n, &s) != EOF)
        {
            if (strcmp(u, username) == 0)
            {
                fclose(f);
                return 0; // Trùng user
            }
        }
        fclose(f);
    }

    f = fopen(ACCOUNT_FILE, "a");
    if (!f)
        return 0;

    // Ghi mặc định 100 điểm
    fprintf(f, "%s %s %s 100\n", username, password, name);
    fclose(f);
    return 1;
}

// Lấy điểm
int get_user_score(char *username)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f)
        return 0;

    char u[32], p[32], n[32];
    int s;
    while (fscanf(f, "%s %s %s %d", u, p, n, &s) != EOF)
    {
        if (strcmp(u, username) == 0)
        {
            fclose(f);
            return s;
        }
    }
    fclose(f);
    return 0;
}

// Cập nhật kết quả thắng thua
void update_game_result(char *winner, char *loser)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f)
        return;

    struct Account
    {
        char u[32], p[32], n[32];
        int s;
    } accounts[100];

    int count = 0;
    while (fscanf(f, "%s %s %s %d", accounts[count].u, accounts[count].p, accounts[count].n, &accounts[count].s) != EOF)
    {
        if (strcmp(accounts[count].u, winner) == 0)
        {
            accounts[count].s += 10; // Thắng +10
        }
        else if (strcmp(accounts[count].u, loser) == 0)
        {
            accounts[count].s -= 10; // Thua -10
            if (accounts[count].s < 0)
                accounts[count].s = 0;
        }
        count++;
        if (count >= 100)
            break;
    }
    fclose(f);

    // Ghi lại file
    f = fopen(ACCOUNT_FILE, "w");
    for (int i = 0; i < count; i++)
    {
        fprintf(f, "%s %s %s %d\n", accounts[i].u, accounts[i].p, accounts[i].n, accounts[i].s);
    }
    fclose(f);
}

// Lấy bảng xếp hạng
typedef struct
{
    char name[32];
    int score;
} PlayerScore;

int compare_score(const void *a, const void *b)
{
    PlayerScore *p1 = (PlayerScore *)a;
    PlayerScore *p2 = (PlayerScore *)b;
    return p2->score - p1->score;
}

void get_leaderboard(char *buffer)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f)
    {
        strcpy(buffer, "Loi doc du lieu.");
        return;
    }

    PlayerScore list[100];
    int count = 0;
    char u[32], p[32], n[32];
    int s;

    while (fscanf(f, "%s %s %s %d", u, p, n, &s) != EOF && count < 100)
    {
        strcpy(list[count].name, n);
        list[count].score = s;
        count++;
    }
    fclose(f);

    qsort(list, count, sizeof(PlayerScore), compare_score);

    strcpy(buffer, ""); // Xóa buffer cũ
    int top = (count < 10) ? count : 10;
    char line[64];
    for (int i = 0; i < top; i++)
    {
        sprintf(line, "%d. %s - %d diem\n", i + 1, list[i].name, list[i].score);
        strcat(buffer, line);
    }
}