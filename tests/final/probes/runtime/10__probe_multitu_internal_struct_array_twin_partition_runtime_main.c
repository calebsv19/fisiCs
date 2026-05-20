#include <stdio.h>

int bucket10_struct_array_twin_step_a(int index, int delta);
int bucket10_struct_array_twin_step_b(int index, int delta);
int bucket10_struct_array_twin_peek_a(void);
int bucket10_struct_array_twin_peek_b(void);

int main(void) {
    (void) bucket10_struct_array_twin_step_a(0, 2);
    (void) bucket10_struct_array_twin_step_b(1, -1);
    printf("%d %d %d\n",
           bucket10_struct_array_twin_step_a(1, 3),
           bucket10_struct_array_twin_step_b(0, 4),
           bucket10_struct_array_twin_peek_a() + bucket10_struct_array_twin_peek_b());
    return 0;
}
