/*
 * Exact hardware-blind artifact metadata validator from immutable os-dev tag
 * edu-28-bounded-mutable-artifact-storage, commit
 * c195bf2b9589c7f94d58375ac8064cbea4316c73.
 *
 * Source: control_kernel.c
 * SHA-256: f26214eaf47d9d189a8f6ed621c2659008ee925dc692f27eefa5ae2a1aa73c57
 */
typedef unsigned char edu21_u8;
typedef unsigned short edu21_u16;
typedef unsigned int edu21_u32;
typedef unsigned long long edu21_u64;

static edu21_u32 read32(const edu21_u8* p) {
    return (edu21_u32)p[0] | ((edu21_u32)p[1] << 8) |
           ((edu21_u32)p[2] << 16) | ((edu21_u32)p[3] << 24);
}

static edu21_u64 read64(const edu21_u8* p) {
    return (edu21_u64)read32(p) | ((edu21_u64)read32(p + 4) << 32);
}

static edu21_u32 read16(const edu21_u8* p) {
    return (edu21_u32)p[0] | ((edu21_u32)p[1] << 8);
}

static edu21_u32 fnv(const edu21_u8* p, edu21_u64 count) {
    edu21_u32 value = 0x811C9DC5U;
    edu21_u64 index;
    for (index = 0; index < count; index = index + 1) {
        value = (value ^ p[index]) * 0x01000193U;
    }
    return value;
}

edu21_u64 edu28_artifact_meta_valid(const edu21_u8* p) {
    edu21_u64 index;
    edu21_u32 state;
    edu21_u32 generation;
    edu21_u32 length;
    edu21_u16 received;
    edu21_u16 chunks;
    edu21_u16 present;
    if (read64(p) != 0x00414D3832554445ULL || read32(p + 8) != 1) return 0;
    if (fnv(p, 508) != read32(p + 508)) return 0;
    state = read32(p + 12);
    generation = read32(p + 16);
    if (read16(p + 20) > 1 || read16(p + 22) != 16) return 0;
    length = read32(p + 24);
    received = read16(p + 40);
    chunks = read16(p + 42);
    if (chunks > 128 || received > chunks) return 0;
    for (index = 60; index < 508; index = index + 1) {
        if (p[index] != 0) return 0;
    }
    if (state == 2 && generation == 0) {
        if (length != 0 || read32(p + 28) != 0x811C9DC5U ||
            read64(p + 32) != 0 || received != 0 || chunks != 0) return 0;
        for (index = 44; index < 60; index = index + 1) {
            if (p[index] != 0) return 0;
        }
        return 2;
    }
    if ((state != 1 && state != 2) || generation == 0 ||
        length == 0 || length > 2048 || read64(p + 32) == 0) return 0;
    if (chunks != (edu21_u16)((length + 15) / 16)) return 0;
    if (state == 2 && received != chunks) return 0;
    present = 0;
    for (index = 0; index < chunks; index = index + 1) {
        if ((p[44 + index / 8] & (1U << (index & 7))) != 0) {
            present = (edu21_u16)(present + 1);
        }
    }
    if (present != received) return 0;
    for (index = chunks; index < 128; index = index + 1) {
        if ((p[44 + index / 8] & (1U << (index & 7))) != 0) return 0;
    }
    return state;
}

edu21_u32 edu28_fnv1a32(const edu21_u8* p, edu21_u64 count) {
    return fnv(p, count);
}
