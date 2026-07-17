#include <stdio.h>

struct wave77_packet {
    double x;
    double y;
    long bias;
};

struct wave77_result {
    long a;
    long b;
    long c;
    long d;
};

typedef struct wave77_result (*wave77_callback_fn)(struct wave77_packet packet,
                                                    int salt,
                                                    long bias);

wave77_callback_fn wave77_factory(int mode);

int main(void) {
    struct wave77_packet packet = {1.5, 2.5, 7};
    wave77_callback_fn left = wave77_factory(4);
    wave77_callback_fn right = wave77_factory(5);
    struct wave77_result a = left(packet, 3, 5);
    struct wave77_result b = right(packet, 3, 5);

    printf("%ld %ld %ld %ld | %ld %ld %ld %ld\n",
           a.a, a.b, a.c, a.d, b.a, b.b, b.c, b.d);
    return 0;
}
