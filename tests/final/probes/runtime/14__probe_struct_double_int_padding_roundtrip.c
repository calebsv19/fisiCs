#include <stdio.h>

typedef struct Mix {
    double d;
    int x;
    char c;
} Mix;

static Mix bounce(Mix in, int bump) {
    Mix out = in;
    out.d = in.d + (double)bump * 0.5;
    out.x = in.x + bump * 3;
    out.c = (char)(in.c + bump);
    return out;
}

int main(void) {
    Mix in = {7.25, 11, 3};
    Mix out = bounce(in, 5);

    long long scaled = (long long)(out.d * 100.0);
    long long off_x = (long long)((char*)&out.x - (char*)&out);
    long long off_c = (long long)((char*)&out.c - (char*)&out);

    printf("%lld %d %d %lld %lld %lld\n",
           scaled, out.x, (int)out.c, off_x, off_c, (long long)sizeof(Mix));
    return 0;
}
