#include <stdio.h>
#include <time.h>

int main(void) {
    struct tm start_tm = (struct tm){0};
    struct tm end_tm = (struct tm){0};
    struct tm end_fmt_tm = (struct tm){0};
    time_t start_time = (time_t)0;
    time_t end_time = (time_t)0;
    long delta = 0;
    char end_buf[32];
    size_t len = 0;

    start_tm.tm_year = 126;
    start_tm.tm_mon = 4;
    start_tm.tm_mday = 26;
    start_tm.tm_hour = 1;
    start_tm.tm_min = 15;
    start_tm.tm_sec = 10;
    start_tm.tm_isdst = 0;

    end_tm.tm_year = 126;
    end_tm.tm_mon = 4;
    end_tm.tm_mday = 26;
    end_tm.tm_hour = 3;
    end_tm.tm_min = 45;
    end_tm.tm_sec = 40;
    end_tm.tm_isdst = 0;
    end_fmt_tm = end_tm;

    start_time = mktime(&start_tm);
    end_time = mktime(&end_tm);
    delta = (long)difftime(end_time, start_time);
    len = strftime(end_buf, sizeof(end_buf), "%H:%M:%S", &end_fmt_tm);

    if (start_time == (time_t)-1 || end_time == (time_t)-1) {
        return 1;
    }
    if (delta != 9030L || len != 8u) {
        return 2;
    }

    printf("delta=%ld end=%s len=%lu\n", delta, end_buf, (unsigned long)len);
    return 0;
}
