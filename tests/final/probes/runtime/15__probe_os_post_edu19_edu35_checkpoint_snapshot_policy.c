/*
 * Source: os-dev queue_kernel.c at immutable tag
 * edu-35-bounded-non-resumable-checkpoint-snapshot, commit
 * 5b39037b8c0a84ef0a80441061b8ed20c3e47693.
 * Origin SHA-256: c26fbc69f48064bfe7b1f5ff0aed1286884d98ba3c94d3b741d1dce208240b7e.
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

/* Hardware-blind validation of the single EDU-35 non-resumable snapshot. */
edu15_u64 edu35_checkpoint_snapshot_valid(const edu15_u8* p) {
    edu15_u64 index;
    edu15_u64 expected_zero;
    edu15_u64 expected_one;
    if (read64(p) != 0x0050433533554445ULL ||
        read16(p + 8) != 1 || read16(p + 10) != 1 ||
        read32(p + 12) != 240 || read32(p + 16) >= 8 ||
        read32(p + 20) == 0 || read64(p + 24) == 0 ||
        read32(p + 32) == 0 || read16(p + 36) != 104 ||
        read16(p + 38) != 3 || read32(p + 40) == 0 ||
        read16(p + 44) != 152 || read16(p + 46) != 3 ||
        read64(p + 48) != 0x3156504D49534445ULL ||
        read64(p + 56) != 0x1E3C373BAF48FAF7ULL ||
        read64(p + 64) > 60000000000ULL ||
        read16(p + 72) < 3 || read16(p + 72) > 4 ||
        read16(p + 74) != read16(p + 72) ||
        (read16(p + 76) != 1 && read16(p + 76) != 2) ||
        read16(p + 78) != 0) {
        return 0;
    }
    expected_zero = read64(p + 80 + 80);
    expected_one = read64(p + 80 + 88);
    for (index = 0; index < 3; index = index + 1) {
        if (read64(p + 184 + index * 8) != expected_zero ||
            read64(p + 208 + index * 8) != expected_one) {
            return 0;
        }
    }
    if (fnv(p + 80, 104) != read32(p + 40) ||
        fnv(p + 80, 152) != read32(p + 232) ||
        fnv(p, 236) != read32(p + 236)) {
        return 0;
    }
    for (index = 240; index < 508; index = index + 1) {
        if (p[index] != 0) return 0;
    }
    return fnv(p, 508) == read32(p + 508);
}
