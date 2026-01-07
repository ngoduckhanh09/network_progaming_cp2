#include "auth.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ACCOUNT_FILE "accounts.txt"

// Struct tạm để xử lý
typedef struct
{
    char u[32], p[32], n[32];
    int s;
} Account;

typedef struct
{
    char name[32];
    int score;
} PlayerScore;

// --- CÁC HÀM CŨ (Check Login, Register, Get Score) GIỮ NGUYÊN ---
int check_login(char *username, char *password)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f)
        return 0;
    char u[32], p[32], n[32];
    int s;
    while (fscanf(f, "%s %s %s %d", u, p, n, &s) != EOF)
    {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0)
        {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int register_user(char *username, char *password, char *name)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (f)
    {
        char u[32], p[32], n[32];
        int s;
        while (fscanf(f, "%s %s %s %d", u, p, n, &s) != EOF)
        {
            if (strcmp(u, username) == 0)
            {
                fclose(f);
                return 0;
            }
        }
        fclose(f);
    }
    f = fopen(ACCOUNT_FILE, "a");
    if (!f)
        return 0;
    fprintf(f, "%s %s %s 100\n", username, password, name);
    fclose(f);
    return 1;
}

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
// ----------------------------------------------------------------

// HÀM CẬP NHẬT ĐIỂM (Đã tối ưu)
void update_game_result(char *winner, char *loser)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f)
        return;

    Account accounts[100];
    int count = 0;

    // 1. Đọc toàn bộ file vào RAM
    while (fscanf(f, "%s %s %s %d", accounts[count].u, accounts[count].p, accounts[count].n, &accounts[count].s) != EOF)
    {
        // Cập nhật điểm ngay lập tức
        if (strcmp(accounts[count].u, winner) == 0)
        {
            accounts[count].s += 10;
        }
        else if (strcmp(accounts[count].u, loser) == 0)
        {
            accounts[count].s -= 10;
            if (accounts[count].s < 0)
                accounts[count].s = 0;
        }

        count++;
        if (count >= 100)
            break;
    }
    fclose(f);

    // 2. Ghi đè lại file ngay lập tức
    if (count > 0)
    {
        f = fopen(ACCOUNT_FILE, "w");
        for (int i = 0; i < count; i++)
        {
            fprintf(f, "%s %s %s %d\n", accounts[i].u, accounts[i].p, accounts[i].n, accounts[i].s);
        }
        fclose(f);
    }
}

// Hàm so sánh cho qsort
int compare_score(const void *a, const void *b)
{
    PlayerScore *p1 = (PlayerScore *)a;
    PlayerScore *p2 = (PlayerScore *)b;
    return p2->score - p1->score;
}

// HÀM LẤY BXH (Đảm bảo Top 10 và không tràn bộ nhớ)
void get_leaderboard(char *buffer)
{
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f)
    {
        strcpy(buffer, "Khong the doc du lieu.");
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

    // Sắp xếp giảm dần
    qsort(list, count, sizeof(PlayerScore), compare_score);

    // Ghi vào buffer (Giới hạn Top 10)
    strcpy(buffer, "");
    int top = (count < 10) ? count : 10; // Chỉ lấy tối đa 10 người
    char line[100];

    for (int i = 0; i < top; i++)
    {
        sprintf(line, "%d. %s - %d diem\n", i + 1, list[i].name, list[i].score);

        // Kiểm tra an toàn: Nếu buffer sắp đầy (gần 1024 bytes) thì dừng
        if (strlen(buffer) + strlen(line) < 1020)
        {
            strcat(buffer, line);
        }
        else
        {
            break;
        }
    }
}