#include <wchar.h>
#include <stdio.h>

int main(void) {
    wchar_t sample[16] = L"wide-zone";
    wchar_t moved[16] = L"abcde";
    wchar_t *dash = wcspbrk(sample, L"-+");
    size_t prefix = wcsspn(sample, L"wide");
    int cmp = wcsncmp(sample, L"wide", 4);
    wmemmove(moved + 2, moved, 3);
    moved[5] = L'\0';
    printf("wchar-span len=%d prefix=%d dash=%d cmp=%d moved=%d%d%d%d%d\n",
           (int)wcslen(sample),
           (int)prefix,
           dash ? (int)(dash - sample) : -1,
           cmp,
           (int)moved[0],
           (int)moved[1],
           (int)moved[2],
           (int)moved[3],
           (int)moved[4]);
    return 0;
}
