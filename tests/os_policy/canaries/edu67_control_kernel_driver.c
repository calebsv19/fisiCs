// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

typedef unsigned char edu67_u8;
typedef unsigned int edu67_u32;
typedef unsigned long long edu67_u64;

extern edu67_u64 edu21_control_validate_request(const edu67_u8 *bytes);

int edu42_program_valid(const edu67_u8 *bytes, edu67_u64 length,
                        edu67_u32 program_id) {
    (void)bytes; (void)length; (void)program_id;
    return 0;
}

double edu42_simulate_damped_partition(double a, double b, double c, edu67_u64 d) {
    (void)a; (void)b; (void)c; (void)d;
    return 0.0;
}

double edu67_simulate_input_bound(double a, double b, double c, edu67_u64 d,
                                  const edu67_u8 *e, edu67_u64 f, edu67_u32 g) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
    return 0.0;
}

static void edu67_write16(edu67_u8 *bytes, unsigned int offset, unsigned int value) {
    bytes[offset] = (edu67_u8)value;
    bytes[offset + 1U] = (edu67_u8)(value >> 8U);
}

static void edu67_write64(edu67_u8 *bytes, unsigned int offset, edu67_u64 value) {
    unsigned int index = 0U;
    while (index < 8U) {
        bytes[offset + index] = (edu67_u8)(value >> (index * 8U));
        index = index + 1U;
    }
}

static edu67_u32 edu67_fnv(const edu67_u8 *bytes, unsigned int count) {
    edu67_u32 value = 0x811C9DC5U;
    unsigned int index = 0U;
    while (index < count) { value = (value ^ bytes[index]) * 0x01000193U; index = index + 1U; }
    return value;
}

static void edu67_seal(edu67_u8 *request) {
    edu67_u32 checksum = edu67_fnv(request, 60U);
    request[60] = (edu67_u8)checksum;
    request[61] = (edu67_u8)(checksum >> 8U);
    request[62] = (edu67_u8)(checksum >> 16U);
    request[63] = (edu67_u8)(checksum >> 24U);
}

static void edu67_bind_request(edu67_u8 *request) {
    unsigned int index = 0U;
    while (index < 64U) { request[index] = 0U; index = index + 1U; }
    request[0] = 'E'; request[1] = 'D'; request[2] = 'U';
    request[3] = '2'; request[4] = '1'; request[5] = 'R'; request[6] = 'Q';
    edu67_write16(request, 8U, 24U);
    request[10] = 44U;
    edu67_write16(request, 12U, 16U);
    edu67_write64(request, 16U, 11ULL);
    edu67_write64(request, 24U, 3ULL);
    edu67_write64(request, 32U, 5ULL);
    edu67_seal(request);
}

int main(void) {
    edu67_u8 request[64];
    edu67_u64 valid;
    edu67_u64 malformed;
    edu67_u64 bad_checksum;
    edu67_bind_request(request);
    valid = edu21_control_validate_request(request);
    edu67_write16(request, 12U, 8U);
    edu67_seal(request);
    malformed = edu21_control_validate_request(request);
    edu67_bind_request(request);
    request[60] ^= 1U;
    bad_checksum = edu21_control_validate_request(request);
    if (valid != 0ULL || malformed != 1ULL || bad_checksum != 2ULL) {
        puts("OS-DEV-EDU67 control canary=result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU67 source=control_kernel.c policy=input-bind-admission result=PASS");
    return 0;
}
