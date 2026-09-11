#include <stdio.h>

typedef unsigned char edu_u8;
typedef unsigned int edu_u32;
typedef unsigned long long edu_u64;
extern edu_u64 edu21_control_validate_request(const edu_u8 *bytes);

static void write16(edu_u8 *b, unsigned int o, unsigned int v) { b[o] = (edu_u8)v; b[o + 1U] = (edu_u8)(v >> 8U); }
static void write32(edu_u8 *b, unsigned int o, edu_u32 v) { unsigned int i = 0U; while (i < 4U) { b[o + i] = (edu_u8)(v >> (i * 8U)); ++i; } }
static void write64(edu_u8 *b, unsigned int o, edu_u64 v) { unsigned int i = 0U; while (i < 8U) { b[o + i] = (edu_u8)(v >> (i * 8U)); ++i; } }
static edu_u32 fnv(const edu_u8 *b) { edu_u32 v = 0x811C9DC5U; unsigned int i = 0U; while (i < 60U) { v = (v ^ b[i]) * 0x01000193U; ++i; } return v; }
static void request55(edu_u8 *b, unsigned int version) {
    unsigned int i = 0U;
    while (i < 64U) { b[i] = 0U; ++i; }
    b[0] = 'E'; b[1] = 'D'; b[2] = 'U'; b[3] = '2'; b[4] = '1'; b[5] = 'R'; b[6] = 'Q';
    write16(b, 8U, version); b[10] = 55U; write16(b, 12U, 32U); write64(b, 16U, 31ULL);
    write16(b, 24U, 0U); b[26] = 4U; b[28] = 'o'; b[29] = 'u'; b[30] = 't'; b[31] = '1';
    write32(b, 60U, fnv(b));
}
int main(void) {
    edu_u8 request[64];
    edu_u64 valid, malformed, unsupported, bad_checksum;
    request55(request, 27U); valid = edu21_control_validate_request(request);
    request[26] = 0U; write32(request, 60U, fnv(request)); malformed = edu21_control_validate_request(request);
    request55(request, 26U); unsupported = edu21_control_validate_request(request);
    request55(request, 27U); request[60] ^= 1U; bad_checksum = edu21_control_validate_request(request);
    if (valid != 0ULL || malformed != 1ULL || unsupported != 3ULL || bad_checksum != 2ULL) { puts("OS-DEV-EDU85 control+simulation result=FAIL"); return 1; }
    puts("OS-DEV-EDU85 source=control+simulation policy=multi-chunk-output result=PASS"); return 0;
}
