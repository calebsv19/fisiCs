#include <stdio.h>
#include <wchar.h>

int main(void) {
    const wchar_t *src = L"abc 123 Z";
    wchar_t letters[8] = {0};
    wchar_t ch = 0;
    int number = 0;
    int count = -1;
    int untouched = 77;
    int m1 = swscanf(src, L"%3ls %3d %lc%n", letters, &number, &ch, &count);
    int m2 = swscanf(L"??", L"%d", &untouched);
    int l0 = (int)letters[0];
    int l2 = (int)letters[2];
    int summary = m1 + number + (int)ch + count + m2 + untouched + l0 + l2;

    printf("wstdio-scan-failure m1=%d letters=%d/%d number=%d ch=%d count=%d m2=%d untouched=%d summary=%d\n",
           m1,
           l0,
           l2,
           number,
           (int)ch,
           count,
           m2,
           untouched,
           summary);

    return m1 == 3 && l0 == L'a' && l2 == L'c' && number == 123 &&
                   ch == L'Z' && count == 9 && m2 == 0 && untouched == 77 &&
                   summary == 498
               ? 0
               : 1;
}
