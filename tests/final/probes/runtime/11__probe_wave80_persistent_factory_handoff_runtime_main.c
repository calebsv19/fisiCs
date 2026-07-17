#include <stdio.h>

typedef long (*wave80_callback_fn)(long value, long generation);

struct wave80_route {
    wave80_callback_fn callback;
    long generation;
};

struct wave80_route wave80_factory(int mode);

int main(void) {
    struct wave80_route first = wave80_factory(0);
    long a = first.callback(5, first.generation);
    struct wave80_route second = wave80_factory(1);
    long b = second.callback(4, second.generation);
    struct wave80_route third = wave80_factory(0);
    long c = third.callback(2, third.generation);

    printf("%ld %ld | %ld %ld | %ld %ld\n",
           first.generation, a,
           second.generation, b,
           third.generation, c);
    return 0;
}
