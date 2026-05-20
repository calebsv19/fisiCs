#include <stdio.h>

int bucket10_struct_twin_step_a(int delta_left, int delta_right);
int bucket10_struct_twin_step_b(int delta_left, int delta_right);
int bucket10_struct_twin_peek_a(void);
int bucket10_struct_twin_peek_b(void);

int main(void) {
    (void) bucket10_struct_twin_step_a(2, 1);
    (void) bucket10_struct_twin_step_b(-2, 3);
    printf("%d %d %d\n",
           bucket10_struct_twin_step_a(1, 2),
           bucket10_struct_twin_step_b(3, -1),
           bucket10_struct_twin_peek_a() + bucket10_struct_twin_peek_b());
    return 0;
}
