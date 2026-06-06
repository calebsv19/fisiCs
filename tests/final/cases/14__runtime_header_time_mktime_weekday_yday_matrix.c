#include <stdio.h>
#include <string.h>
#include <time.h>

int main(void) {
    struct tm tmv = {0};
    time_t t = (time_t)0;
    char out[48];
    size_t len = 0;
    int yday = 0;
    int wday = 0;
    int mon = 0;
    int mday = 0;
    long summary = 0;

    tmv.tm_year = 126;
    tmv.tm_mon = 1;
    tmv.tm_mday = 28;
    tmv.tm_hour = 23;
    tmv.tm_min = 59;
    tmv.tm_sec = 61;
    tmv.tm_isdst = 0;

    t = mktime(&tmv);
    if (t == (time_t)-1) {
        return 1;
    }

    yday = tmv.tm_yday;
    wday = tmv.tm_wday;
    mon = tmv.tm_mon;
    mday = tmv.tm_mday;
    len = strftime(out, sizeof(out), "%Y-%m-%d %H:%M:%S %j %w", &tmv);
    summary = (long)len + yday + wday + mon + mday + tmv.tm_sec + (long)out[0];

    printf("time-mktime-weekday out=%s len=%lu yday=%d wday=%d mon=%d mday=%d sec=%d summary=%ld\n",
           out,
           (unsigned long)len,
           yday,
           wday,
           mon,
           mday,
           tmv.tm_sec,
           summary);

    return len == 25u && strcmp(out, "2026-03-01 00:00:01 060 0") == 0 &&
                   yday == 59 && wday == 0 && mon == 2 && mday == 1 &&
                   tmv.tm_sec == 1 && summary == 138L
               ? 0
               : 1;
}
