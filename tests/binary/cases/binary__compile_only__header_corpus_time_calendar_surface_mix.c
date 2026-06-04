#include <stddef.h>
#include <time.h>

static int wave32_time_calendar_surface(void) {
    struct tm tmv = {0};
    char out[32];
    time_t epoch = (time_t)0;
    clock_t ticks = (clock_t)0;
    size_t len = 0;

    tmv.tm_year = 126;
    tmv.tm_mon = 0;
    tmv.tm_mday = 4;
    tmv.tm_wday = 0;
    tmv.tm_yday = 3;
    tmv.tm_hour = 6;
    tmv.tm_min = 7;
    tmv.tm_sec = 8;
    tmv.tm_isdst = 0;
    len = strftime(out, sizeof(out), "%a %b %j", &tmv);

    return len > 0u && epoch == (time_t)0 && ticks == (clock_t)0 && CLOCKS_PER_SEC > 0 ? 0 : 1;
}

int main(void) {
    return wave32_time_calendar_surface();
}
