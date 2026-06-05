#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

int main(void) {
    mbstate_t state;
    wchar_t z_wc = 0;
    wchar_t r_wc = 0;
    char out[8];
    int q_out = 0;
    int s_out = 0;
    int mb_cur = 0;
    size_t len_a = 0;
    size_t len_empty = 0;
    size_t conv_z = 0;
    size_t emit_q = 0;
    int mb_r = 0;
    int wc_s = 0;

    if (!setlocale(LC_ALL, "C")) {
        return 1;
    }
    mb_cur = (int)MB_CUR_MAX;

    memset(&state, 0, sizeof(state));
    len_a = mbrlen("A", 4, &state);
    memset(&state, 0, sizeof(state));
    len_empty = mbrlen("", 1, &state);
    memset(&state, 0, sizeof(state));
    conv_z = mbrtowc(&z_wc, "Z", 4, &state);
    memset(&state, 0, sizeof(state));
    emit_q = wcrtomb(out, L'Q', &state);
    out[emit_q < sizeof(out) ? emit_q : 0] = '\0';
    q_out = (int)out[0];
    mb_r = mbtowc(&r_wc, "R", MB_CUR_MAX);
    wc_s = wctomb(out, L'S');
    s_out = (int)out[0];

    printf("locale-wchar-mb mbcur=%d lena=%lu empty=%lu conv=%lu wc=%d emit=%lu out=%d mbtowc=%d wc2=%d wctomb=%d out2=%d\n",
           mb_cur,
           (unsigned long)len_a,
           (unsigned long)len_empty,
           (unsigned long)conv_z,
           (int)z_wc,
           (unsigned long)emit_q,
           q_out,
           mb_r,
           (int)r_wc,
           wc_s,
           s_out);

    return mb_cur == 1 && len_a == 1u && len_empty == 0u && conv_z == 1u &&
                   z_wc == L'Z' && emit_q == 1u && mb_r == 1 && r_wc == L'R' &&
                   wc_s == 1 &&
                   q_out == 'Q' && s_out == 'S'
               ? 0
               : 1;
}
