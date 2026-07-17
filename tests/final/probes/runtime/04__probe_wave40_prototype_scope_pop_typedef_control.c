#include <stdio.h>

typedef int wave40_scope_t;

int wave40_scope_probe(int wave40_scope_t);

static wave40_scope_t wave40_identity(wave40_scope_t value) {
    return value;
}

int main(void) {
    int value = wave40_identity(7);
    printf("%d\n", value);
    return value != 7;
}
