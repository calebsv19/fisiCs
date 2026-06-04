#include <stdio.h>
#include <string.h>
#include <time.h>

int main(void) {
    struct tm start_tm = {0};
    struct tm end_tm = {0};
    struct tm norm_tm = {0};
    time_t start_time = (time_t)0;
    time_t end_time = (time_t)0;
    time_t norm_time = (time_t)0;
    long delta = 0;
    char norm_buf[32];
    size_t len = 0;

    start_tm.tm_year = 126;
    start_tm.tm_mon = 0;
    start_tm.tm_mday = 1;
    start_tm.tm_hour = 0;
    start_tm.tm_min = 0;
    start_tm.tm_sec = 0;
    start_tm.tm_isdst = 0;

    end_tm = start_tm;
    end_tm.tm_mday = 2;
    end_tm.tm_hour = 12;
    end_tm.tm_min = 30;

    norm_tm.tm_year = 126;
    norm_tm.tm_mon = 0;
    norm_tm.tm_mday = 32;
    norm_tm.tm_hour = 1;
    norm_tm.tm_min = 2;
    norm_tm.tm_sec = 3;
    norm_tm.tm_isdst = 0;

    start_time = mktime(&start_tm);
    end_time = mktime(&end_tm);
    norm_time = mktime(&norm_tm);
    if (start_time == (time_t)-1 || end_time == (time_t)-1 || norm_time == (time_t)-1) {
        return 1;
    }

    delta = (long)difftime(end_time, start_time);
    len = strftime(norm_buf, sizeof(norm_buf), "%Y-%m-%d %H:%M", &norm_tm);

    printf("time-normal delta=%ld norm=%s len=%lu\n", delta, norm_buf, (unsigned long)len);
    return delta == 131400L && len == 16u && strcmp(norm_buf, "2026-02-01 01:02") == 0 ? 0 : 1;
}
