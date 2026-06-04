#include <locale.h>
#include <string.h>
#include <time.h>

int wave32_locale_time_deep_bridge(char *out, unsigned long cap) {
    struct tm tmv = {0};
    struct lconv *conv = 0;
    size_t len = 0;

    if (!setlocale(LC_ALL, "C")) {
        return -1;
    }
    conv = localeconv();
    if (!conv || !conv->decimal_point || strcmp(conv->decimal_point, ".") != 0) {
        return -2;
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
    len = strftime(out, (size_t)cap, "%Y-%m-%d:%j", &tmv);
    return (int)len;
}
