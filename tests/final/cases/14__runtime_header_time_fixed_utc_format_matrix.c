#include <stdio.h>
#include <string.h>
#include <time.h>

int main(void) {
    time_t epoch = (time_t)0;
    time_t later = (time_t)3661;
    struct tm *utc = gmtime(&epoch);
    struct tm fixed = {0};
    char utc_buf[32];
    char fixed_buf[64];
    size_t utc_len = 0;
    size_t fixed_len = 0;
    long diff = 0;
    int summary = 0;

    if (!utc) {
        return 1;
    }

    utc_len = strftime(utc_buf, sizeof(utc_buf), "%Y-%m-%d %H:%M:%S", utc);

    fixed.tm_year = 126;
    fixed.tm_mon = 5;
    fixed.tm_mday = 5;
    fixed.tm_wday = 5;
    fixed.tm_yday = 155;
    fixed.tm_hour = 12;
    fixed.tm_min = 34;
    fixed.tm_sec = 56;
    fixed.tm_isdst = 0;
    fixed_len = strftime(fixed_buf, sizeof(fixed_buf), "%A|%B|%j|%U|%W|%H%M%S", &fixed);
    diff = (long)difftime(later, epoch);
    summary = (int)utc_len + (int)fixed_len + (int)diff + (int)utc_buf[0] + (int)fixed_buf[0];

    printf("time-fixed-utc utc=%s len=%lu fixed=%s len=%lu diff=%ld summary=%d\n",
           utc_buf,
           (unsigned long)utc_len,
           fixed_buf,
           (unsigned long)fixed_len,
           diff,
           summary);

    return utc_len == 19u && strcmp(utc_buf, "1970-01-01 00:00:00") == 0 &&
                   fixed_len == 28u &&
                   strcmp(fixed_buf, "Friday|June|156|22|22|123456") == 0 &&
                   diff == 3661L && summary == 3827
               ? 0
               : 1;
}
