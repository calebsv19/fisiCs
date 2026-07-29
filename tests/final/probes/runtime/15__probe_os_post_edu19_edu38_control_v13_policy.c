/*
 * Source: os-dev control_kernel.c at immutable tag
 * edu-38-bounded-runner-context-instancing, commit
 * 59d622af0278e0a57a36285ea7b75839a352bc41.
 * Origin SHA-256: 83fe45bc8023c0352e91cb63a30a7fcd9ed0846683699970acce6b0483aac86d.
 * The complete source below is unchanged from that snapshot.
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
    edu21_u16 payload;
    edu21_u16 expected;
    edu21_u8 operation;
    if (read64(p) != 0x0051523132554445ULL) return EDU21_ERR_FORMAT;
    if (read16(p + 8) != 13 || p[11] != 0 || read16(p + 14) != 0) {
        return EDU21_ERR_FORMAT;
    }
    payload = read16(p + 12);
    operation = p[10];
    if (payload > 32 || read64(p + 16) == 0) return EDU21_ERR_FORMAT;
    for (index = 56; index < 60; index = index + 1) {
        if (p[index] != 0) return EDU21_ERR_FORMAT;
    }
    if (fnv(p, 60) != read32(p + 60)) return EDU21_ERR_CHECKSUM;
    if (operation < 1 || operation > 20) return EDU21_ERR_UNSUPPORTED;
    expected = 0;
    if (operation == 3) expected = 28;
    if (operation == 4 || operation == 6 ||
        operation == 7 || operation == 10) expected = 8;
    if (operation == 8) expected = 12;
    if (operation == 9) expected = 10;
    if (operation == 11) expected = 12;
    if (operation == 12) expected = 24;
    if (operation == 13 || operation == 16) expected = 4;
    if (operation == 15) expected = 6;
    if (operation == 18 || operation == 19) expected = 12;
    if (operation == 20) expected = 8;
    if (payload != expected) return EDU21_ERR_FORMAT;
    if ((operation == 4 || operation == 6 || operation == 7 ||
         operation == 8 || operation == 9 || operation == 10 ||
         operation == 18 || operation == 19 || operation == 20) &&
        (read64(p + 24) == 0 || read64(p + 24) > 0xffffffffULL)) {
        return EDU21_ERR_FORMAT;
    }
    for (index = 24 + payload; index < 56; index = index + 1) {
        if (p[index] != 0) return EDU21_ERR_FORMAT;
    }
    if (operation == 3 &&
        (read16(p + 32) == 0 || read16(p + 32) > 4)) {
        return EDU21_ERR_FORMAT;
    }
    if (operation == 3 &&
        (read16(p + 34) != 104 || read32(p + 36) == 0 ||
         read32(p + 40) == 0)) {
        return EDU21_ERR_FORMAT;
    }
    if (operation == 3 &&
        (read64(p + 44) == 0 || read64(p + 44) > 60000000000ULL)) {
        return EDU21_ERR_FORMAT;
    }
    if (operation == 11 &&
        (read32(p + 28) == 0 || read32(p + 28) > 2048)) {
        return EDU21_ERR_FORMAT;
    }
    if ((operation == 12 || operation == 13 || operation == 15 ||
         operation == 16) && read32(p + 24) == 0) {
        return EDU21_ERR_FORMAT;
    }
    if (operation == 12) {
        if (read16(p + 28) >= 128 || p[30] == 0 || p[30] > 16 ||
            p[31] != 0) return EDU21_ERR_FORMAT;
        for (index = 32 + p[30]; index < 48; index = index + 1) {
            if (p[index] != 0) return EDU21_ERR_FORMAT;
        }
    }
    if (operation == 15 && read16(p + 28) >= 128) {
        return EDU21_ERR_FORMAT;
    }
    if (operation == 18 &&
        (read16(p + 32) >= 7 || read16(p + 34) != 0)) {
        return EDU21_ERR_FORMAT;
    }
    if (operation == 19 &&
        (read16(p + 32) >= 20 || read16(p + 34) != 0)) {
        return EDU21_ERR_FORMAT;
    }
    return EDU21_OK;
}

/* EDU-32 retains EDU-28's physical transaction but narrows it to workload v1. */
edu21_u64 edu28_artifact_meta_valid(const edu21_u8* p) {
    edu21_u64 index;
    edu21_u32 state;
    edu21_u32 generation;
    edu21_u32 length;
    edu21_u16 received;
    edu21_u16 chunks;
    edu21_u16 present;
    if (read64(p) != 0x004D573233554445ULL || read32(p + 8) != 2) return 0;
    if (fnv(p, 508) != read32(p + 508)) return 0;
    state = read32(p + 12);
    generation = read32(p + 16);
    if (read16(p + 20) > 1 || read16(p + 22) != 16) return 0;
    length = read32(p + 24);
    received = read16(p + 40);
    chunks = read16(p + 42);
    if (chunks > 128 || received > chunks) return 0;
    if (read16(p + 60) != 1 || read16(p + 62) != 1) return 0;
    for (index = 64; index < 508; index = index + 1) {
        if (p[index] != 0) return 0;
    }
    if ((state != 1 && state != 2) || generation == 0 ||
        length != 104 || read32(p + 28) == 0 ||
        read64(p + 32) == 0) return 0;
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
