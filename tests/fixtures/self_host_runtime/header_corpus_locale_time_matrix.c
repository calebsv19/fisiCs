#include <locale.h>
#include <stdio.h>
#include <time.h>

int main(void) {
    struct lconv *conv = 0;
    struct tm start_tm = (struct tm){0};
    struct tm end_tm = (struct tm){0};
    struct tm end_fmt_tm = (struct tm){0};
    time_t start_time = (time_t)0;
    time_t end_time = (time_t)0;
    long delta = 0;
    char date_buf[32];
    char time_buf[32];
    size_t date_len = 0;
    size_t time_len = 0;

    if (!setlocale(LC_ALL, "C")) {
        return 1;
    }
    conv = localeconv();
    if (!conv || !conv->decimal_point || conv->decimal_point[0] != '.') {
        return 2;
    }

    end_fmt_tm.tm_year = 126;
    end_fmt_tm.tm_mon = 4;
    end_fmt_tm.tm_mday = 26;
    end_fmt_tm.tm_hour = 14;
    end_fmt_tm.tm_min = 7;
    end_fmt_tm.tm_sec = 5;
    end_fmt_tm.tm_isdst = 0;

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

    start_time = mktime(&start_tm);
    end_time = mktime(&end_tm);
    delta = (long)difftime(end_time, start_time);
    date_len = strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &end_fmt_tm);
    time_len = strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &end_tm);

    if (start_time == (time_t)-1 || end_time == (time_t)-1) {
        return 3;
    }
    if (delta != 9030L || date_len != 19u || time_len != 8u) {
        return 4;
    }

    printf(
        "date=%s decimal=%s delta=%ld end=%s lens=%lu,%lu\n",
        date_buf,
        conv->decimal_point,
        delta,
        time_buf,
        (unsigned long)date_len,
        (unsigned long)time_len);
    return 0;
}
