#include <stdio.h>

int bucket10_twin_step_a(int x);
int bucket10_twin_step_b(int x);
int bucket10_twin_peek_a(void);
int bucket10_twin_peek_b(void);

int main(void) {
    (void) bucket10_twin_step_a(4);
    (void) bucket10_twin_step_b(6);
    printf("%d %d %d\n",
           bucket10_twin_step_a(1),
           bucket10_twin_step_b(2),
           bucket10_twin_peek_a() + bucket10_twin_peek_b());
    return 0;
}
