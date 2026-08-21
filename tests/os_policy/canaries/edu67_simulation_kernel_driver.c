// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

typedef unsigned char edu67_u8;
typedef unsigned int edu67_u32;
typedef unsigned long long edu67_u64;

extern double edu42_simulate_damped_partition(double, double, double, edu67_u64);
extern double edu67_simulate_input_bound(double, double, double, edu67_u64,
                                         const edu67_u8 *, edu67_u64, edu67_u32);

static edu67_u32 edu67_fnv(const edu67_u8 *bytes, edu67_u64 count) {
    edu67_u32 value = 0x811C9DC5U;
    edu67_u64 index = 0ULL;
    while (index < count) {
        value = (value ^ bytes[index]) * 0x01000193U;
        index = index + 1ULL;
    }
    return value;
}

int main(void) {
    edu67_u8 input[256];
    edu67_u64 index = 0ULL;
    edu67_u32 checksum;
    double expected;
    double valid;
    double bad_checksum;
    double short_input;

    while (index < 256ULL) {
        input[index] = (edu67_u8)(index * 13ULL + 7ULL);
        index = index + 1ULL;
    }
    checksum = edu67_fnv(input, 256ULL);
    expected = edu42_simulate_damped_partition(1.0, 2.0, 0.5, 3ULL);
    valid = edu67_simulate_input_bound(1.0, 2.0, 0.5, 3ULL,
                                       input, 256ULL, checksum);
    bad_checksum = edu67_simulate_input_bound(1.0, 2.0, 0.5, 3ULL,
                                              input, 256ULL, checksum ^ 1U);
    short_input = edu67_simulate_input_bound(1.0, 2.0, 0.5, 3ULL,
                                             input, 255ULL, checksum);
    if (valid != expected || bad_checksum != 0.0 || short_input != 0.0) {
        puts("OS-DEV-EDU67 simulation canary=result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU67 source=simulation_kernel.c policy=input-bound-result result=PASS");
    return 0;
}
