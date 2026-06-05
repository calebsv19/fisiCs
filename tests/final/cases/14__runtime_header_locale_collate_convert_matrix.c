#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

int main(void) {
    const char *src = "abc";
    const char *src_cursor = src;
    wchar_t wide[8];
    const wchar_t *wide_cursor = wide;
    char narrow[8];
    size_t xlen = 0;
    size_t wlen = 0;
    size_t nlen = 0;
    int coll_lt = 0;
    int coll_eq = 0;
    char key[8];

    if (!setlocale(LC_ALL, "C")) {
        return 1;
    }

    coll_lt = strcoll("abc", "abd") < 0 ? 1 : 0;
    coll_eq = strcoll("same", "same") == 0 ? 1 : 0;
    xlen = strxfrm(key, "abc", sizeof(key));
    wlen = mbsrtowcs(wide, &src_cursor, 8, 0);
    nlen = wcsrtombs(narrow, (const wchar_t **)&wide_cursor, sizeof(narrow), 0);

    printf("locale-collate lt=%d eq=%d xlen=%lu key=%s wlen=%lu wide=%d%d%d nlen=%lu narrow=%s done=%d\n",
           coll_lt,
           coll_eq,
           (unsigned long)xlen,
           key,
           (unsigned long)wlen,
           (int)wide[0],
           (int)wide[1],
           (int)wide[2],
           (unsigned long)nlen,
           narrow,
           src_cursor == 0 && wide_cursor == 0 ? 1 : 0);

    return coll_lt == 1 && coll_eq == 1 && xlen == 3u && strcmp(key, "abc") == 0 &&
                   wlen == 3u && wide[0] == L'a' && wide[1] == L'b' &&
                   wide[2] == L'c' && nlen == 3u && strcmp(narrow, "abc") == 0 &&
                   src_cursor == 0 && wide_cursor == 0
               ? 0
               : 1;
}
