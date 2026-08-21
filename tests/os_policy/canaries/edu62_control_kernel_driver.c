// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

typedef unsigned char edu62_u8;
typedef unsigned short edu62_u16;
typedef unsigned int edu62_u32;
typedef unsigned long long edu62_u64;

extern edu62_u64 edu21_control_validate_request(const edu62_u8 *bytes);

int edu42_program_valid(const edu62_u8 *bytes, edu62_u64 length,
                        edu62_u32 program_id) {
    (void)bytes;
    (void)length;
    (void)program_id;
    return 0;
}

double edu42_simulate_damped_partition(double left, double right, double damping,
                                       edu62_u64 steps) {
    (void)left;
    (void)right;
    (void)damping;
    (void)steps;
    return 0.0;
}

static void edu62_write16(edu62_u8 *bytes, unsigned int offset,
                          edu62_u16 value) {
    bytes[offset] = (edu62_u8)value;
    bytes[offset + 1U] = (edu62_u8)(value >> 8U);
}

static void edu62_write64(edu62_u8 *bytes, unsigned int offset,
                          edu62_u64 value) {
    unsigned int index = 0U;
    while (index < 8U) {
        bytes[offset + index] = (edu62_u8)(value >> (index * 8U));
        index = index + 1U;
    }
}

static edu62_u32 edu62_fnv(const edu62_u8 *bytes, unsigned int count) {
    edu62_u32 value = 0x811C9DC5U;
    unsigned int index = 0U;
    while (index < count) {
        value = (value ^ bytes[index]) * 0x01000193U;
        index = index + 1U;
    }
    return value;
}

static void edu62_seal(edu62_u8 *request) {
    edu62_u32 checksum = edu62_fnv(request, 60U);
    request[60] = (edu62_u8)checksum;
    request[61] = (edu62_u8)(checksum >> 8U);
    request[62] = (edu62_u8)(checksum >> 16U);
    request[63] = (edu62_u8)(checksum >> 24U);
}

static void edu62_valid_output_retrieve(edu62_u8 *request) {
    unsigned int index = 0U;
    while (index < 64U) {
        request[index] = 0U;
        index = index + 1U;
    }
    request[0] = 'E';
    request[1] = 'D';
    request[2] = 'U';
    request[3] = '2';
    request[4] = '1';
    request[5] = 'R';
    request[6] = 'Q';
    request[7] = 0U;
    edu62_write16(request, 8U, 22U);
    request[10] = 43U;
    edu62_write16(request, 12U, 20U);
    edu62_write64(request, 16U, 7ULL);
    edu62_write64(request, 24U, 9ULL);
    edu62_write16(request, 32U, 3U);
    edu62_write64(request, 36U, 5ULL);
    edu62_seal(request);
}

int main(void) {
    edu62_u8 request[64];
    edu62_u64 valid;
    edu62_u64 bad_payload;
    edu62_u64 bad_reserved;
    edu62_u64 bad_checksum;

    edu62_valid_output_retrieve(request);
    valid = edu21_control_validate_request(request);
    edu62_write16(request, 32U, 10U);
    edu62_seal(request);
    bad_payload = edu21_control_validate_request(request);
    edu62_valid_output_retrieve(request);
    request[34] = 1U;
    edu62_seal(request);
    bad_reserved = edu21_control_validate_request(request);
    edu62_valid_output_retrieve(request);
    request[60] ^= 1U;
    bad_checksum = edu21_control_validate_request(request);
    if (valid != 0ULL || bad_payload != 1ULL || bad_reserved != 1ULL ||
        bad_checksum != 2ULL) {
        puts("OS-DEV-EDU62 canary=result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU62 source=control_kernel.c policy=output-retrieve-validation result=PASS");
    return 0;
}
