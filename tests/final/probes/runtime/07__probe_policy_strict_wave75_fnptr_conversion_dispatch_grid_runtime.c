#include <stdio.h>

typedef int (*convert_fn)(unsigned char, signed char);

static int widen_add(unsigned char raw, signed char bias) {
    return (int)(unsigned char)(raw + (unsigned char)bias);
}

static int signed_mix(unsigned char raw, signed char bias) {
    return (int)(signed char)((signed char)raw - bias);
}

static int xor_fold(unsigned char raw, signed char bias) {
    return (int)(unsigned char)((unsigned char)(raw ^ (unsigned char)bias) + 7u);
}

static convert_fn choose(int mode, int flip) {
    convert_fn table[3] = {widen_add, signed_mix, xor_fold};
    convert_fn first = mode == 0 ? table[0] : table[1];
    return flip ? table[2] : first;
}

static int run(convert_fn (*chooser)(int, int), int mode, int flip, unsigned char raw, signed char bias) {
    convert_fn selected = chooser(mode, flip);
    convert_fn fallback = selected ? selected : widen_add;
    return fallback(raw, bias);
}

int main(void) {
    convert_fn (*chooser)(int, int) = choose;
    int first = run(chooser, 0, 0, 240u, 22);
    int second = run(chooser, 1, 0, 130u, -17);
    int third = run(chooser, 2, 1, 51u, -4);
    printf("%d %d %d\n", first, second, third);
    return 0;
}
