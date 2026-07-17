#include <stdio.h>

enum Token {
    TOKEN_NEG = -4,
    TOKEN_SMALL = 9,
    TOKEN_BIG = 130
};

static int fold_enum(enum Token token, unsigned char payload, signed char adjust) {
    int signed_path = (int)token + (int)adjust;
    unsigned int unsigned_path = (unsigned int)(unsigned char)(payload + token);
    unsigned long wide_path = (unsigned long)((unsigned int)payload + (unsigned int)TOKEN_BIG);
    int compare_path = token < (unsigned int)1 ? 17 : 3;
    return signed_path + (int)unsigned_path + (int)(wide_path & 31ul) + compare_path;
}

int main(void) {
    printf("%d %d\n",
           fold_enum(TOKEN_NEG, 250u, -3),
           fold_enum(TOKEN_SMALL, 12u, 5));
    return 0;
}
