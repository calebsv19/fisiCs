#include <stdio.h>

typedef unsigned char edu_u8;
typedef unsigned int edu_u32;
typedef unsigned long long edu_u64;

extern double edu67_simulate_input_bound(double, double, double, edu_u64,
                                         const edu_u8 *, edu_u64, edu_u32,
                                         edu_u8 *, edu_u64);
extern double edu78_simulate_relaxed_partition(double, double, double, edu_u64);

static edu_u32 fnv(const edu_u8 *bytes, unsigned int count) {
    edu_u32 value = 0x811C9DC5U;
    unsigned int index = 0U;
    while (index < count) { value = (value ^ bytes[index]) * 0x01000193U; ++index; }
    return value;
}

int main(void) {
    edu_u8 input[256];
    edu_u8 output[2048] = {0};
    unsigned int index = 0U;
    while (index < 256U) { input[index] = (edu_u8)(index * 5U + 7U); ++index; }
    if (edu78_simulate_relaxed_partition(10.0, 8.0, 2.0, 2ULL) != 11.375 ||
            edu67_simulate_input_bound(10.0, 4.0, 0.5, 3ULL, input, 256ULL,
                fnv(input, 256U), output, 2048ULL) == 0.0 ||
            output[127] != (edu_u8)(input[127] ^ 0xd8U)) {
        puts("OS-DEV-EDU78 control+queue+simulation result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU78 source=control+queue+simulation policy=third-program result=PASS");
    return 0;
}
