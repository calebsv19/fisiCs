#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(void) {
    struct tm tmv = (struct tm){0};
    struct lconv* conv = 0;
    char date[32];
    size_t len = 0;

    setlocale(LC_ALL, "C");
    conv = localeconv();

    tmv.tm_year = 126;
    tmv.tm_mon = 4;
    tmv.tm_mday = 26;
    tmv.tm_hour = 14;
    tmv.tm_min = 7;
    tmv.tm_sec = 5;
    tmv.tm_isdst = 0;

    len = strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &tmv);
    if (!conv || len != 19u || strcmp(date, "2026-05-26 14:07:05") != 0) {
        return 1;
    }
    if (!conv->decimal_point || strcmp(conv->decimal_point, ".") != 0) {
        return 2;
    }

    printf("date=%s decimal=%s len=%lu\n", date, conv->decimal_point, (unsigned long)len);
    return 0;
}
