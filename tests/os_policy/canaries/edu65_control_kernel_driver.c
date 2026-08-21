// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

typedef unsigned char edu65_u8;
typedef unsigned int edu65_u32;
typedef unsigned long long edu65_u64;

extern edu65_u64 edu21_control_validate_request(const edu65_u8 *bytes);

int edu42_program_valid(const edu65_u8 *bytes, edu65_u64 length,
                        edu65_u32 program_id) {
    (void)bytes;
    (void)length;
    (void)program_id;
    return 0;
}

double edu42_simulate_damped_partition(double left, double right, double damping,
                                       edu65_u64 steps) {
    (void)left;
    (void)right;
    (void)damping;
    (void)steps;
    return 0.0;
}

static void edu65_write16(edu65_u8 *bytes, unsigned int offset,
                          unsigned int value) {
    bytes[offset] = (edu65_u8)value;
    bytes[offset + 1U] = (edu65_u8)(value >> 8U);
}

static void edu65_write64(edu65_u8 *bytes, unsigned int offset,
                          edu65_u64 value) {
    unsigned int index = 0U;
    while (index < 8U) {
        bytes[offset + index] = (edu65_u8)(value >> (index * 8U));
        index = index + 1U;
    }
}

static edu65_u32 edu65_fnv(const edu65_u8 *bytes, unsigned int count) {
    edu65_u32 value = 0x811C9DC5U;
    unsigned int index = 0U;
    while (index < count) {
        value = (value ^ bytes[index]) * 0x01000193U;
        index = index + 1U;
    }
    return value;
}

static void edu65_seal(edu65_u8 *request) {
    edu65_u32 checksum = edu65_fnv(request, 60U);
    request[60] = (edu65_u8)checksum;
    request[61] = (edu65_u8)(checksum >> 8U);
    request[62] = (edu65_u8)(checksum >> 16U);
    request[63] = (edu65_u8)(checksum >> 24U);
}

static void edu65_output_retrieve(edu65_u8 *request, unsigned int slot) {
    unsigned int index = 0U;
    while (index < 64U) {
        request[index] = 0U;
        index = index + 1U;
    }
    request[0] = 'E'; request[1] = 'D'; request[2] = 'U';
    request[3] = '2'; request[4] = '1'; request[5] = 'R'; request[6] = 'Q';
    edu65_write16(request, 8U, 23U);
    request[10] = 43U;
    edu65_write16(request, 12U, 20U);
    edu65_write64(request, 16U, 7ULL);
    edu65_write64(request, 24U, 9ULL);
    edu65_write16(request, 32U, 3U);
    edu65_write16(request, 34U, slot);
    edu65_write64(request, 36U, 5ULL);
    edu65_seal(request);
}

int main(void) {
    edu65_u8 request[64];
    edu65_u64 valid_slot_one;
    edu65_u64 invalid_slot;
    edu65_u64 bad_checksum;

    edu65_output_retrieve(request, 1U);
    valid_slot_one = edu21_control_validate_request(request);
    edu65_output_retrieve(request, 2U);
    invalid_slot = edu21_control_validate_request(request);
    edu65_output_retrieve(request, 0U);
    request[60] ^= 1U;
    bad_checksum = edu21_control_validate_request(request);
    if (valid_slot_one != 0ULL || invalid_slot != 1ULL ||
        bad_checksum != 2ULL) {
        puts("OS-DEV-EDU65 canary=result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU65 source=control_kernel.c policy=slot-aware-output-retrieve result=PASS");
    return 0;
}
