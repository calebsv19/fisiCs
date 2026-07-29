/*
 * Historical control-frame admission mirror for immutable os-dev EDU-24,
 * EDU-26, EDU-28, EDU-29, EDU-30, and EDU-31 tags.
 *
 * Exact control_kernel.c SHA-256 values:
 *   EDU-24 50ef645698dd009c308ddcfb92f6bde9babd0ed0c46a2ebe2f25239f5c4ba55e
 *   EDU-26 0fcc0c6b4c03f6569ab0ed2e98f5fc9929eeba7b870d4c2a2a5e60cb619ae458
 *   EDU-28 f26214eaf47d9d189a8f6ed621c2659008ee925dc692f27eefa5ae2a1aa73c57
 *   EDU-29 d861ef05b48dfccb0723bb837c4709a0cb16d526922718db89d7ccf0571c3ccc
 *   EDU-30 dab8ce7b06dc97b7abc31395b4a60655844dac4e0eac00a43b94a575f87a55cb
 *   EDU-31 fd510a6e9b3dd25238451eec1ec8696b42176eb95d8be768911e73b368684295
 *
 * This model retains each frozen version's exact payload and reserved-byte
 * rules. Serial I/O and operation execution remain assembly-owned.
 */
typedef unsigned char edu_wire_u8;
typedef unsigned short edu_wire_u16;
typedef unsigned int edu_wire_u32;
typedef unsigned long long edu_wire_u64;

enum {
    EDU_WIRE_OK = 0,
    EDU_WIRE_ERR_FORMAT = 1,
    EDU_WIRE_ERR_CHECKSUM = 2,
    EDU_WIRE_ERR_UNSUPPORTED = 3
};

static edu_wire_u16 edu_wire_read16(const edu_wire_u8 *p) {
    return (edu_wire_u16)((edu_wire_u16)p[0] |
                          ((edu_wire_u16)p[1] << 8));
}

static edu_wire_u32 edu_wire_read32(const edu_wire_u8 *p) {
    return (edu_wire_u32)p[0] | ((edu_wire_u32)p[1] << 8) |
           ((edu_wire_u32)p[2] << 16) | ((edu_wire_u32)p[3] << 24);
}

static edu_wire_u64 edu_wire_read64(const edu_wire_u8 *p) {
    return (edu_wire_u64)edu_wire_read32(p) |
           ((edu_wire_u64)edu_wire_read32(p + 4) << 32);
}

static edu_wire_u32 edu_wire_fnv(
    const edu_wire_u8 *p, edu_wire_u64 count) {
    edu_wire_u32 hash = 2166136261U;
    edu_wire_u64 index;
    for (index = 0; index < count; index = index + 1) {
        hash = (hash ^ p[index]) * 16777619U;
    }
    return hash;
}

static edu_wire_u16 edu_wire_expected_payload(
    edu_wire_u16 version, edu_wire_u8 operation) {
    if (version == 2) {
        if (operation == 3) return 8;
        if (operation == 8) return 4;
        if (operation == 9) return 10;
        return 0;
    }
    if (operation == 3) return version >= 6 ? 10 : 8;
    if (operation == 4 || operation == 6 ||
        operation == 7 || operation == 10) return 8;
    if (operation == 8) return 12;
    if (operation == 9) return 10;
    if (version >= 4) {
        if (operation == 11) return 12;
        if (operation == 12) return 24;
        if (operation == 13 || operation == 16) return 4;
        if (operation == 15) return 6;
    }
    return 0;
}

static edu_wire_u64 edu_wire_validate(
    const edu_wire_u8 *p,
    edu_wire_u16 version,
    edu_wire_u8 maximum_operation) {
    edu_wire_u64 index;
    edu_wire_u16 payload;
    edu_wire_u16 expected;
    edu_wire_u8 operation;
    if (p == (const edu_wire_u8 *)0) return EDU_WIRE_ERR_FORMAT;
    if (edu_wire_read64(p) != 0x0051523132554445ULL) {
        return EDU_WIRE_ERR_FORMAT;
    }
    if (edu_wire_read16(p + 8) != version ||
        p[11] != 0 || edu_wire_read16(p + 14) != 0) {
        return EDU_WIRE_ERR_FORMAT;
    }
    payload = edu_wire_read16(p + 12);
    operation = p[10];
    if (payload > 32 || edu_wire_read64(p + 16) == 0) {
        return EDU_WIRE_ERR_FORMAT;
    }
    for (index = 56; index < 60; index = index + 1) {
        if (p[index] != 0) return EDU_WIRE_ERR_FORMAT;
    }
    if (edu_wire_fnv(p, 60) != edu_wire_read32(p + 60)) {
        return EDU_WIRE_ERR_CHECKSUM;
    }
    if (operation < 1 || operation > maximum_operation) {
        return EDU_WIRE_ERR_UNSUPPORTED;
    }
    expected = edu_wire_expected_payload(version, operation);
    if (payload != expected) return EDU_WIRE_ERR_FORMAT;

    if (version == 2) {
        if (operation == 9) {
            for (index = 34; index < 56; index = index + 1) {
                if (p[index] != 0) return EDU_WIRE_ERR_FORMAT;
            }
        }
        return EDU_WIRE_OK;
    }

    if ((operation == 4 || operation == 6 || operation == 7 ||
         operation == 8 || operation == 9 || operation == 10) &&
        (edu_wire_read64(p + 24) == 0 ||
         edu_wire_read64(p + 24) > 0xffffffffULL)) {
        return EDU_WIRE_ERR_FORMAT;
    }
    for (index = 24 + payload; index < 56; index = index + 1) {
        if (p[index] != 0) return EDU_WIRE_ERR_FORMAT;
    }
    if (version >= 4) {
        if (operation == 11 &&
            (edu_wire_read32(p + 28) == 0 ||
             edu_wire_read32(p + 28) > 2048)) {
            return EDU_WIRE_ERR_FORMAT;
        }
        if ((operation == 12 || operation == 13 ||
             operation == 15 || operation == 16) &&
            edu_wire_read32(p + 24) == 0) {
            return EDU_WIRE_ERR_FORMAT;
        }
        if (operation == 12) {
            if (edu_wire_read16(p + 28) >= 128 ||
                p[30] == 0 || p[30] > 16 || p[31] != 0) {
                return EDU_WIRE_ERR_FORMAT;
            }
            for (index = 32 + p[30]; index < 48; index = index + 1) {
                if (p[index] != 0) return EDU_WIRE_ERR_FORMAT;
            }
        }
        if (operation == 15 && edu_wire_read16(p + 28) >= 128) {
            return EDU_WIRE_ERR_FORMAT;
        }
    }
    if (version >= 6 && operation == 3 &&
        (edu_wire_read16(p + 32) == 0 ||
         edu_wire_read16(p + 32) > 4)) {
        return EDU_WIRE_ERR_FORMAT;
    }
    return EDU_WIRE_OK;
}

edu_wire_u64 edu24_wire_v2_valid(const edu_wire_u8 *p) {
    return edu_wire_validate(p, 2, 9);
}

edu_wire_u64 edu26_wire_v3_valid(const edu_wire_u8 *p) {
    return edu_wire_validate(p, 3, 10);
}

edu_wire_u64 edu28_wire_v4_valid(const edu_wire_u8 *p) {
    return edu_wire_validate(p, 4, 16);
}

edu_wire_u64 edu29_wire_v5_valid(const edu_wire_u8 *p) {
    return edu_wire_validate(p, 5, 16);
}

edu_wire_u64 edu30_wire_v6_valid(const edu_wire_u8 *p) {
    return edu_wire_validate(p, 6, 16);
}

edu_wire_u64 edu31_wire_v7_valid(const edu_wire_u8 *p) {
    return edu_wire_validate(p, 7, 17);
}
