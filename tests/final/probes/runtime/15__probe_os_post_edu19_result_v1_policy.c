/*
 * Source: os-dev queue_kernel.c at immutable tag
 * edu-33-bounded-typed-result-payload, commit
 * 49e430415a4ec2bfa45823d8ecd1a98b6a28dd51.
 * Origin SHA-256: 1f4e6ddb6b905619e5ca1f00f1e63c90d4e2818677ebe6cbf023f6f9ce9f43aa.
 * The extracted typedefs, helpers, and validator below are unchanged from
 * that snapshot.
 */
typedef unsigned char edu15_u8;
typedef unsigned int edu15_u32;
typedef unsigned long long edu15_u64;

static edu15_u32 read16(const edu15_u8* p) {
    return (edu15_u32)p[0] | ((edu15_u32)p[1] << 8);
}

static edu15_u32 read32(const edu15_u8* p) {
    return (edu15_u32)p[0] | ((edu15_u32)p[1] << 8) |
           ((edu15_u32)p[2] << 16) | ((edu15_u32)p[3] << 24);
}

static edu15_u64 read64(const edu15_u8* p) {
    return (edu15_u64)read32(p) | ((edu15_u64)read32(p + 4) << 32);
}

static edu15_u32 fnv(const edu15_u8* p, edu15_u64 count) {
    edu15_u32 value = 0x811C9DC5U;
    edu15_u64 index;
    for (index = 0; index < count; index = index + 1) value = (value ^ p[index]) * 0x01000193U;
    return value;
}

/* Hardware-blind validation of one fixed EDU-33 typed result sector. */
edu15_u64 edu33_result_payload_valid(const edu15_u8* p) {
    edu15_u64 index;
    if (read64(p) != 0x0031523333554445ULL ||
        read16(p + 8) != 1 || read16(p + 10) != 1 ||
        read32(p + 12) != 80 || read32(p + 16) >= 8 ||
        read32(p + 20) == 0 || read64(p + 24) == 0 ||
        read32(p + 32) == 0 || read16(p + 36) != 104 ||
        read16(p + 38) != 0 || read32(p + 40) == 0 ||
        read32(p + 44) != 0 || read32(p + 76) != 0) {
        return 0;
    }
    if (fnv(p, 72) != read32(p + 72)) return 0;
    for (index = 80; index < 508; index = index + 1) {
        if (p[index] != 0) return 0;
    }
    return fnv(p, 508) == read32(p + 508);
}
