#include <stdio.h>
#include <wchar.h>

int main(void) {
    wchar_t text[] = L"delta::omega:theta";
    wchar_t *state = 0;
    wchar_t *tok1 = wcstok(text, L":", &state);
    wchar_t *tok2 = wcstok(0, L":", &state);
    wchar_t *tok3 = wcstok(0, L":", &state);
    wchar_t *tok4 = wcstok(0, L":", &state);
    wchar_t *tail = tok3 ? wcsrchr(tok3, L't') : 0;
    wchar_t *pick = tok2 ? wmemchr(tok2, L'g', 5) : 0;
    int len1 = tok1 ? (int)wcslen(tok1) : -1;
    int len2 = tok2 ? (int)wcslen(tok2) : -1;
    int len3 = tok3 ? (int)wcslen(tok3) : -1;
    int tail_pos = tail ? (int)(tail - tok3) : -1;
    int pick_pos = pick ? (int)(pick - tok2) : -1;
    int span = tok1 ? (int)wcsspn(tok1, L"abcdefghijklmnopqrstuvwxyz") : -1;
    int final = tok4 == 0 ? 1 : 0;
    int summary = len1 * 13 + len2 * 7 + len3 * 5 + tail_pos * 3 +
                  pick_pos * 2 + span + final;

    printf("wchar-token lens=%d,%d,%d tail=%d pick=%d span=%d final=%d summary=%d\n",
           len1,
           len2,
           len3,
           tail_pos,
           pick_pos,
           span,
           final,
           summary);

    return len1 == 5 && len2 == 5 && len3 == 5 && tail_pos == 3 &&
                   pick_pos == 3 && span == 5 && final == 1 &&
                   summary == 146
               ? 0
               : 1;
}
