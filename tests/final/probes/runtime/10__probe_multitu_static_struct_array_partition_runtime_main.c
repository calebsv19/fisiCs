#include <stdio.h>

int bucket10_local_struct_array_step_a(int index, int delta);
int bucket10_local_struct_array_step_b(int index, int delta);
int bucket10_local_struct_array_peek_a(void);
int bucket10_local_struct_array_peek_b(void);

int main(void) {
    (void) bucket10_local_struct_array_step_a(1, 3);
    (void) bucket10_local_struct_array_step_b(0, -2);
    printf("%d %d %d\n",
           bucket10_local_struct_array_step_a(2, 1),
           bucket10_local_struct_array_step_b(1, 4),
           bucket10_local_struct_array_peek_a() + bucket10_local_struct_array_peek_b());
    return 0;
}
