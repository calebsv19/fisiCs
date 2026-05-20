#include <stdio.h>

int bucket10_internal_function_step_a(int base);
int bucket10_internal_function_step_b(int base);

int main(void) {
    printf("%d %d %d\n",
           bucket10_internal_function_step_a(4),
           bucket10_internal_function_step_b(4),
           bucket10_internal_function_step_a(0) + bucket10_internal_function_step_b(0));
    return 0;
}
