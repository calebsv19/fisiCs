#include <stdio.h>

struct wave79_state {
    long accumulator;
    int calls;
};

struct wave79_payload {
    double x;
    long bias;
    int lane;
};

struct wave79_result {
    long a;
    long b;
    long c;
    long d;
};

typedef struct wave79_result (*wave79_callback_fn)(struct wave79_state *state,
                                                    struct wave79_payload payload,
                                                    long salt);

struct wave79_table {
    wave79_callback_fn first;
    wave79_callback_fn second;
    long epoch;
    struct wave79_state *state;
};

struct wave79_table wave79_make_table(struct wave79_state *state, int mode);

int main(void) {
    struct wave79_state state = {5, 0};
    struct wave79_payload payload = {2.5, 4, 3};
    struct wave79_table table = wave79_make_table(&state, 2);
    struct wave79_result first = table.first(table.state, payload, 6);
    struct wave79_result second = table.second(table.state, payload, 2);

    printf("%ld %ld %ld %ld %ld | %ld %ld %ld %ld\n",
           table.epoch,
           first.a, first.b, first.c, first.d,
           second.a, second.b, second.c, second.d);
    return 0;
}
