#include <locale.h>
#include <stddef.h>
#include <time.h>

struct HeaderCorpusCalendarView {
    struct tm stamp;
    const char* fmt;
};

static size_t describe_calendar(struct HeaderCorpusCalendarView view, char* out, size_t cap) {
    struct lconv* conv = localeconv();
    clock_t ticks = (clock_t)0;

    if (conv == 0 || cap == 0) {
        return 0;
    }

    (void)ticks;
    return strftime(out, cap, view.fmt, &view.stamp);
}

int main(void) {
    struct HeaderCorpusCalendarView view;
    char out[32];

    setlocale(LC_ALL, "C");
    view.stamp.tm_year = 126;
    view.stamp.tm_mon = 4;
    view.stamp.tm_mday = 26;
    view.stamp.tm_hour = 14;
    view.stamp.tm_min = 7;
    view.stamp.tm_sec = 5;
    view.fmt = "%Y-%m-%d";

    return describe_calendar(view, out, sizeof(out)) == 0;
}
