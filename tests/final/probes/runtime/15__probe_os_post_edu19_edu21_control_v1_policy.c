/*
 * Exact control_kernel.c from immutable os-dev tag
 * edu-21-bounded-serial-host-control, commit
 * d1544b0dbb0a2dfd06272e6e7ef049fb44281310.
 */
typedef unsigned char edu21_u8;
typedef unsigned short edu21_u16;
typedef unsigned int edu21_u32;
typedef unsigned long long edu21_u64;

enum {
    EDU21_OK = 0,
    EDU21_ERR_FORMAT = 1,
    EDU21_ERR_CHECKSUM = 2,
    EDU21_ERR_UNSUPPORTED = 3
};

static edu21_u16 read16(const edu21_u8* p) {
    return (edu21_u16)p[0] | ((edu21_u16)p[1] << 8);
}

static edu21_u32 read32(const edu21_u8* p) {
    return (edu21_u32)p[0] | ((edu21_u32)p[1] << 8) |
           ((edu21_u32)p[2] << 16) | ((edu21_u32)p[3] << 24);
}

static edu21_u64 read64(const edu21_u8* p) {
    return (edu21_u64)read32(p) | ((edu21_u64)read32(p + 4) << 32);
}

static edu21_u32 fnv(const edu21_u8* p, edu21_u64 count) {
    edu21_u32 value = 0x811C9DC5U;
    edu21_u64 index;
    for (index = 0; index < count; index = index + 1) {
        value = (value ^ p[index]) * 0x01000193U;
    }
    return value;
}

/* Hardware-blind validation of one fixed EDU-21 request frame. */
edu21_u64 edu21_control_validate_request(const edu21_u8* p) {
    edu21_u64 index;
    if (read64(p) != 0x0051523132554445ULL) return EDU21_ERR_FORMAT;
    if (read16(p + 8) != 1 || p[11] != 0 || read16(p + 14) != 0) {
        return EDU21_ERR_FORMAT;
    }
    if (read16(p + 12) > 32 || read64(p + 16) == 0) return EDU21_ERR_FORMAT;
    for (index = 56; index < 60; index = index + 1) {
        if (p[index] != 0) return EDU21_ERR_FORMAT;
    }
    if (fnv(p, 60) != read32(p + 60)) return EDU21_ERR_CHECKSUM;
    if (p[10] < 1 || p[10] > 6) return EDU21_ERR_UNSUPPORTED;
    if ((p[10] == 3 && read16(p + 12) != 8) ||
        (p[10] == 4 && read16(p + 12) != 0) ||
        (p[10] != 3 && p[10] != 4 && read16(p + 12) != 0)) {
        return EDU21_ERR_FORMAT;
    }
    return EDU21_OK;
}
