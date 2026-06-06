#include <stdio.h>
#include <wchar.h>

int main(void) {
    wchar_t out[64];
    wchar_t word[16] = {0};
    int value = 0;
    int count = -1;
    int n = swprintf(out, 64, L"%ls:%04d:%lc", L"wide", 27, L'Z');
    int m = swscanf(out, L"%15ls%n", word, &count);
    int colon = (int)out[4];
    int zed = (int)out[10];
    int w0 = (int)word[0];
    int w4 = (int)word[4];
    int ok_scan = swscanf(L"  -42 tail", L"%d", &value);
    int summary = n + m + count + colon + zed + w0 + w4 + value + ok_scan;

    printf("wstdio-format-scan n=%d colon=%d zed=%d m=%d count=%d word0=%d word4=%d value=%d ok=%d summary=%d\n",
           n,
           colon,
           zed,
           m,
           count,
           w0,
           w4,
           value,
           ok_scan,
           summary);

    return n == 11 && colon == ':' && zed == 'Z' && m == 1 && count == 11 &&
                   w0 == L'w' && w4 == L':' && value == -42 && ok_scan == 1 &&
                   summary == 307
               ? 0
               : 1;
}
