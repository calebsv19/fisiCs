#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(void) {
    struct tm tmv = {0};
    struct lconv *conv = 0;
    char out[64];
    size_t len = 0;

    if (!setlocale(LC_ALL, "C")) {
        return 1;
    }
    conv = localeconv();
    if (!conv || !conv->decimal_point || !conv->thousands_sep) {
        return 2;
    }

    tmv.tm_year = 126;
    tmv.tm_mon = 0;
    tmv.tm_mday = 4;
    tmv.tm_wday = 0;
    tmv.tm_yday = 3;
    tmv.tm_hour = 6;
    tmv.tm_min = 7;
    tmv.tm_sec = 8;
    tmv.tm_isdst = 0;

    len = strftime(out, sizeof(out), "%a %b %d %Y %H:%M:%S %j", &tmv);
    printf("locale-time names=%s len=%lu decimal=%s\n", out, (unsigned long)len, conv->decimal_point);
    return len == 28u && strcmp(out, "Sun Jan 04 2026 06:07:08 004") == 0 && strcmp(conv->decimal_point, ".") == 0 ? 0 : 1;
}
