// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

typedef unsigned long long edu48_u64;
typedef unsigned int edu48_u32;
typedef unsigned char edu48_u8;

extern int edu32_workload_valid(const edu48_u8 *bytes, edu48_u64 length);
extern int edu42_program_valid(const edu48_u8 *bytes, edu48_u64 length,
                               edu48_u32 program_id);
extern edu48_u64 edu42_program_result(const edu48_u8 *bytes, edu48_u64 length,
                                      edu48_u32 program_id);
extern edu48_u64 edu12_reduce_result(edu48_u64 left_bits,
                                     edu48_u64 right_bits,
                                     edu48_u64 seed);

static void edu48_write16(edu48_u8 *bytes, unsigned int offset,
                          unsigned int value) {
    bytes[offset] = (edu48_u8)value;
    bytes[offset + 1U] = (edu48_u8)(value >> 8U);
}

static void edu48_write32(edu48_u8 *bytes, unsigned int offset,
                          edu48_u32 value) {
    unsigned int index = 0U;
    while (index < 4U) {
        bytes[offset + index] = (edu48_u8)(value >> (index * 8U));
        index = index + 1U;
    }
}

static void edu48_write64(edu48_u8 *bytes, unsigned int offset,
                          edu48_u64 value) {
    unsigned int index = 0U;
    while (index < 8U) {
        bytes[offset + index] = (edu48_u8)(value >> (index * 8U));
        index = index + 1U;
    }
}

int main(void) {
    edu48_u8 workload[104] = {0};
    const edu48_u64 seed = 0x0123456789ABCDEFULL;
    const edu48_u64 program_one_left = 0x4024000000000000ULL;
    const edu48_u64 program_one_right = 0x4022000000000000ULL;
    const edu48_u64 program_two_left = 0x400E800000000000ULL;
    const edu48_u64 program_two_right = 0x4008000000000000ULL;
    edu48_u64 expected_one;
    edu48_u64 expected_two;

    workload[0] = 'E';
    workload[1] = 'D';
    workload[2] = 'U';
    workload[3] = '3';
    workload[4] = '2';
    workload[5] = 'W';
    workload[6] = '1';
    edu48_write16(workload, 8U, 1U);
    edu48_write16(workload, 10U, 1U);
    edu48_write32(workload, 12U, 104U);
    edu48_write64(workload, 16U, 0x3FF0000000000000ULL);
    edu48_write64(workload, 24U, 0x4000000000000000ULL);
    edu48_write64(workload, 32U, 0x3FE0000000000000ULL);
    edu48_write64(workload, 40U, 0x0000000000000000ULL);
    edu48_write64(workload, 48U, 0x3FF0000000000000ULL);
    edu48_write64(workload, 56U, 0x3FF0000000000000ULL);
    edu48_write64(workload, 64U, 3ULL);
    edu48_write64(workload, 72U, seed);
    edu48_write64(workload, 80U, program_one_left);
    edu48_write64(workload, 88U, program_one_right);
    expected_one = edu12_reduce_result(program_one_left, program_one_right, seed);
    expected_two = edu12_reduce_result(program_two_left, program_two_right, seed);
    edu48_write64(workload, 96U, expected_one);

    if (!edu32_workload_valid(workload, 104ULL)
            || !edu42_program_valid(workload, 104ULL, 1U)
            || !edu42_program_valid(workload, 104ULL, 2U)
            || edu42_program_result(workload, 104ULL, 1U) != expected_one
            || edu42_program_result(workload, 104ULL, 2U) != expected_two
            || edu42_program_valid(workload, 104ULL, 3U)
            || edu42_program_result(workload, 104ULL, 3U) != 0ULL) {
        puts("OS-DEV-EDU48 canary=result=FAIL");
        return 1;
    }
    workload[72] ^= 1U;
    if (edu32_workload_valid(workload, 104ULL)
            || edu42_program_valid(workload, 103ULL, 1U)) {
        puts("OS-DEV-EDU48 canary=result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU48 source=simulation_kernel.c policy=bundle-selection result=PASS");
    return 0;
}
