#include <string.h>
#include "auth.h"

int check_login(const char *username, const char *password)
{
    // Logic đơn giản: Chỉ cần password là "123" là cho qua
    if (strcmp(password, "123") == 0)
    {
        return 1;
    }
    return 0;
}