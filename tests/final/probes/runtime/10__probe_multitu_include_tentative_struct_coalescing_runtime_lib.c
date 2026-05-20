#include "10__probe_multitu_include_tentative_struct_coalescing_runtime.h"

void bucket10_header_struct_prime(int left, int right) {
    bucket10_header_struct.left = left;
    bucket10_header_struct.right = right;
}

int bucket10_header_struct_shift_left(int delta) {
    bucket10_header_struct.left += delta;
    return bucket10_header_struct.left;
}

int bucket10_header_struct_shift_right(int delta) {
    bucket10_header_struct.right += delta;
    return bucket10_header_struct.right;
}

int bucket10_header_struct_total(void) {
    return bucket10_header_struct.left + bucket10_header_struct.right;
}
