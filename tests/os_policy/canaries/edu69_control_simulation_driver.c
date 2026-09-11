#include <stdio.h>

typedef unsigned char edu_u8;
typedef unsigned int edu_u32;
typedef unsigned long long edu_u64;

extern double edu67_simulate_input_bound(double, double, double, edu_u64,
                                         const edu_u8 *, edu_u64, edu_u32,
                                         edu_u8 *, edu_u64);

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
    double result;
    while (index < 256U) { input[index] = (edu_u8)(index * 3U + 1U); ++index; }
    result = edu67_simulate_input_bound(10.0, 4.0, 0.5, 3ULL, input, 256ULL,
                                        fnv(input, 256U), output, 2048ULL);
    if (result == 0.0 || output[0] != (edu_u8)(input[0] ^ 0x69U) ||
            output[255] != (edu_u8)(input[255] ^ 0x58U) ||
            edu67_simulate_input_bound(10.0, 4.0, 0.5, 3ULL, input, 256ULL,
                fnv(input, 256U), output, 2047ULL) != 0.0) {
        puts("OS-DEV-EDU69 control+simulation result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU69 source=control+simulation policy=static-output result=PASS");
    return 0;
}
