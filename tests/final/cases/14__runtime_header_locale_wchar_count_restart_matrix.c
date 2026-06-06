#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

int main(void) {
    const char *src = "ab cd";
    const char *count_cursor = src;
    const char *conv_cursor = src;
    wchar_t wide[8];
    const wchar_t *wide_cursor = wide;
    char narrow[8];
    mbstate_t st;
    size_t count = 0;
    size_t conv = 0;
    size_t back_count = 0;
    size_t back = 0;
    size_t wlen = 0;
    int sum = 0;

    if (!setlocale(LC_ALL, "C")) {
        return 1;
    }

    memset(&st, 0, sizeof(st));
    count = mbsrtowcs(0, &count_cursor, 0, &st);

    memset(&st, 0, sizeof(st));
    conv = mbsrtowcs(wide, &conv_cursor, 8, &st);
    wlen = wcslen(wide);

    memset(&st, 0, sizeof(st));
    back_count = wcsrtombs(0, &wide_cursor, 0, &st);

    wide_cursor = wide;
    memset(&st, 0, sizeof(st));
    back = wcsrtombs(narrow, &wide_cursor, sizeof(narrow), &st);

    sum = (int)count + (int)conv + (int)back_count + (int)back +
          (int)wlen + (int)wide[0] + (int)wide[3] + (int)narrow[4];

    printf("locale-wchar-count count=%lu conv=%lu wlen=%lu w0=%d w3=%d cursor=%d back_count=%lu back=%lu narrow=%s done=%d sum=%d\n",
           (unsigned long)count,
           (unsigned long)conv,
           (unsigned long)wlen,
           (int)wide[0],
           (int)wide[3],
           conv_cursor == 0 ? 1 : 0,
           (unsigned long)back_count,
           (unsigned long)back,
           narrow,
           wide_cursor == 0 ? 1 : 0,
           sum);

    return count == 5u && conv == 5u && wlen == 5u && wide[0] == L'a' &&
                   wide[3] == L'c' && conv_cursor == 0 && back_count == 5u &&
                   back == 5u && strcmp(narrow, "ab cd") == 0 &&
                   wide_cursor == 0 && sum == 321
               ? 0
               : 1;
}
