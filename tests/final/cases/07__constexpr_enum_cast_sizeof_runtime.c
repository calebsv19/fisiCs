#include <stdio.h>

enum {
    E0 = (int)sizeof(int),
    E1 = (int)((unsigned)E0 * 3u),
    E2 = (int)(E1 + (int)sizeof(char[5]))
};

static int g_arr[E2];
static int g_init = (int)(sizeof(g_arr) / sizeof(g_arr[0])) + E1;

static int select_case(int v) {
    switch (v) {
        case (int)sizeof(int):
            return 7;
        case (int)(E1 + 1):
            return 11;
        default:
            return 13;
    }
}

int main(void) {
    int n = (int)(sizeof(g_arr) / sizeof(g_arr[0]));
    int out = n + g_init + select_case((int)sizeof(int)) + select_case(E1 + 1) + select_case(0);
    printf("%d\n", out);
    return 0;
}
