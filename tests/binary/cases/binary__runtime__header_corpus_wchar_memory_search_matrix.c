#include <wchar.h>
#include <stdio.h>

int main(void) {
    wchar_t dst[8];
    const wchar_t src[] = L"wide";
    wchar_t *hit;
    wmemset(dst, L'_', 7);
    dst[7] = L'\0';
    wmemcpy(dst, src, 5);
    hit = wcschr(dst, L'd');
    printf("len=%d diff=%d last=%d cmp=%d\n",
           (int)wcslen(dst),
           hit ? (int)(hit - dst) : -1,
           (int)dst[3],
           wcscmp(dst, L"wide"));
    return 0;
}
