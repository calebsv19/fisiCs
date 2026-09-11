#include <stdio.h>

typedef unsigned char edu_u8;
typedef unsigned int edu_u32;
typedef unsigned long long edu_u64;

extern edu_u64 edu21_control_validate_request(const edu_u8 *bytes);

static void write16(edu_u8 *bytes, unsigned int offset, unsigned int value) {
    bytes[offset] = (edu_u8)value;
    bytes[offset + 1U] = (edu_u8)(value >> 8U);
}

static void write32(edu_u8 *bytes, unsigned int offset, edu_u32 value) {
    unsigned int index = 0U;
    while (index < 4U) { bytes[offset + index] = (edu_u8)(value >> (index * 8U)); ++index; }
}

static void write64(edu_u8 *bytes, unsigned int offset, edu_u64 value) {
    unsigned int index = 0U;
    while (index < 8U) { bytes[offset + index] = (edu_u8)(value >> (index * 8U)); ++index; }
}

static edu_u32 fnv(const edu_u8 *bytes, unsigned int count) {
    edu_u32 value = 0x811C9DC5U;
    unsigned int index = 0U;
    while (index < count) { value = (value ^ bytes[index]) * 0x01000193U; ++index; }
    return value;
}

static void seal(edu_u8 *request) { write32(request, 60U, fnv(request, 60U)); }

static void request47(edu_u8 *request, unsigned int version) {
    unsigned int index = 0U;
    while (index < 64U) { request[index] = 0U; ++index; }
    request[0] = 'E'; request[1] = 'D'; request[2] = 'U';
    request[3] = '2'; request[4] = '1'; request[5] = 'R'; request[6] = 'Q';
    write16(request, 8U, version);
    request[10] = 47U;
    write16(request, 12U, 32U);
    write64(request, 16U, 17ULL);
    write64(request, 32U, 29ULL);
    write32(request, 40U, 256U);
    write32(request, 44U, 0x1037U);
    write32(request, 48U, 0x8042U);
    write32(request, 52U, 0x99abU);
    seal(request);
}

int main(void) {
    edu_u8 request[64];
    edu_u64 valid;
    edu_u64 malformed;
    edu_u64 unsupported;
    edu_u64 bad_checksum;
    request47(request, 26U);
    valid = edu21_control_validate_request(request);
    write32(request, 40U, 255U);
    seal(request);
    malformed = edu21_control_validate_request(request);
    request47(request, 25U);
    unsupported = edu21_control_validate_request(request);
    request47(request, 26U);
    request[60] ^= 1U;
    bad_checksum = edu21_control_validate_request(request);
    if (valid != 0ULL || malformed != 1ULL || unsupported != 3ULL || bad_checksum != 2ULL) {
        puts("OS-DEV-EDU83 control+simulation result=FAIL");
        return 1;
    }
    puts("OS-DEV-EDU83 source=control+simulation policy=multi-chunk-object result=PASS");
    return 0;
}
