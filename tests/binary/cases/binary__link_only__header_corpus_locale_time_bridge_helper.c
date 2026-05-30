#include <locale.h>
#include <stddef.h>
#include <time.h>

size_t header_corpus_format_calendar(char* out, size_t cap) {
    struct tm tmv = {0};

    setlocale(LC_ALL, "C");
    tmv.tm_year = 126;
    tmv.tm_mon = 4;
    tmv.tm_mday = 26;
    tmv.tm_hour = 14;
    tmv.tm_min = 7;
    tmv.tm_sec = 5;

    if (out == 0 || cap == 0) {
        return 19u;
    }
    return strftime(out, cap, "%Y-%m-%d %H:%M:%S", &tmv);
}
