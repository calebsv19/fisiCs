#include <stdio.h>

int bucket10_local_array_step_a(int index, int delta);
int bucket10_local_array_step_b(int index, int delta);
int bucket10_local_array_peek_a(void);
int bucket10_local_array_peek_b(void);

int main(void) {
    (void) bucket10_local_array_step_a(1, 4);
    (void) bucket10_local_array_step_b(2, -1);
    printf("%d %d %d\n",
           bucket10_local_array_step_a(0, 1),
           bucket10_local_array_step_b(1, 2),
           bucket10_local_array_peek_a() + bucket10_local_array_peek_b());
    return 0;
}
