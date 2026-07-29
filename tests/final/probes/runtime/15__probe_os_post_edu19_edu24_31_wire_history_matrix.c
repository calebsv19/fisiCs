typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

extern u64 edu24_wire_v2_valid(const u8 *p);
extern u64 edu26_wire_v3_valid(const u8 *p);
extern u64 edu28_wire_v4_valid(const u8 *p);
extern u64 edu29_wire_v5_valid(const u8 *p);
extern u64 edu30_wire_v6_valid(const u8 *p);
extern u64 edu31_wire_v7_valid(const u8 *p);
extern int printf(const char *format, ...);

static void put16(u8 *p, u32 offset, u16 value) {
    p[offset] = (u8)value;
    p[offset + 1U] = (u8)(value >> 8);
}

static void put32(u8 *p, u32 offset, u32 value) {
    p[offset] = (u8)value;
    p[offset + 1U] = (u8)(value >> 8);
    p[offset + 2U] = (u8)(value >> 16);
    p[offset + 3U] = (u8)(value >> 24);
}

static void put64(u8 *p, u32 offset, u64 value) {
    u32 index;
    for (index = 0; index < 8U; index = index + 1U) {
        p[offset + index] = (u8)(value >> (index * 8U));
    }
}

static u32 fnv(const u8 *p, u32 count) {
    u32 hash = 2166136261U;
    u32 index;
    for (index = 0; index < count; index = index + 1U) {
        hash = (hash ^ p[index]) * 16777619U;
    }
    return hash;
}

static void zero64(u8 *p) {
    u32 index;
    for (index = 0; index < 64U; index = index + 1U) p[index] = 0U;
}

static u16 expected_payload(u16 version, u8 operation) {
    if (version == 2U) {
        if (operation == 3U) return 8U;
        if (operation == 8U) return 4U;
        if (operation == 9U) return 10U;
        return 0U;
    }
    if (operation == 3U) return version >= 6U ? 10U : 8U;
    if (operation == 4U || operation == 6U ||
        operation == 7U || operation == 10U) return 8U;
    if (operation == 8U) return 12U;
    if (operation == 9U) return 10U;
    if (version >= 4U) {
        if (operation == 11U) return 12U;
        if (operation == 12U) return 24U;
        if (operation == 13U || operation == 16U) return 4U;
        if (operation == 15U) return 6U;
    }
    return 0U;
}

static void seal(u8 *p) {
    put32(p, 60U, fnv(p, 60U));
}

static void build(u8 *p, u16 version, u8 operation) {
    u16 payload = expected_payload(version, operation);
    u32 index;
    zero64(p);
    put64(p, 0U, 0x0051523132554445ULL);
    put16(p, 8U, version);
    p[10U] = operation;
    put16(p, 12U, payload);
    put64(p, 16U, 0xED24000000000001ULL + operation);
    if (version >= 3U &&
        (operation == 4U || operation == 6U || operation == 7U ||
         operation == 8U || operation == 9U || operation == 10U)) {
        put64(p, 24U, 7ULL);
    }
    if (version >= 4U) {
        if (operation == 11U) put32(p, 28U, 16U);
        if (operation == 12U) {
            put32(p, 24U, 9U);
            put16(p, 28U, 3U);
            p[30U] = 4U;
            for (index = 0; index < 4U; index = index + 1U) {
                p[32U + index] = (u8)(0xA0U + index);
            }
        }
        if (operation == 13U || operation == 16U) put32(p, 24U, 9U);
        if (operation == 15U) {
            put32(p, 24U, 9U);
            put16(p, 28U, 3U);
        }
    }
    if (version >= 6U && operation == 3U) put16(p, 32U, 1U);
    seal(p);
}

static u64 validate(u16 version, const u8 *p) {
    if (version == 2U) return edu24_wire_v2_valid(p);
    if (version == 3U) return edu26_wire_v3_valid(p);
    if (version == 4U) return edu28_wire_v4_valid(p);
    if (version == 5U) return edu29_wire_v5_valid(p);
    if (version == 6U) return edu30_wire_v6_valid(p);
    return edu31_wire_v7_valid(p);
}

static u8 max_operation(u16 version) {
    if (version == 2U) return 9U;
    if (version == 3U) return 10U;
    if (version < 7U) return 16U;
    return 17U;
}

static int checks;
static u32 digest = 2166136261U;

static int expect_case(u32 id, u64 actual, u64 expected) {
    checks = checks + 1;
    digest = (digest ^ id) * 16777619U;
    digest = (digest ^ (u32)actual) * 16777619U;
    digest = (digest ^ (u32)(actual >> 32U)) * 16777619U;
    return actual == expected ? 0 : (int)id;
}

int main(void) {
    u8 frame[64];
    u16 version;
    u8 operation;
    u32 id = 1U;
    int failure;

#define RUN(ACTUAL, EXPECTED) \
    do { \
        failure = expect_case(id, (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
        id = id + 1U; \
    } while (0)

    for (version = 2U; version <= 7U; version = version + 1U) {
        for (operation = 1U; operation <= max_operation(version);
             operation = operation + 1U) {
            build(frame, version, operation);
            RUN(validate(version, frame), 0U);
        }
    }

    for (version = 2U; version <= 7U; version = version + 1U) {
        build(frame, version, 1U);
        put16(frame, 8U, (u16)(version + 1U)); seal(frame);
        RUN(validate(version, frame), 1U);
        build(frame, version, (u8)(max_operation(version) + 1U));
        RUN(validate(version, frame), 3U);
        build(frame, version, 1U); frame[60U] ^= 1U;
        RUN(validate(version, frame), 2U);
        build(frame, version, 1U); frame[0U] ^= 1U;
        RUN(validate(version, frame), 1U);
        build(frame, version, 1U); put64(frame, 16U, 0ULL); seal(frame);
        RUN(validate(version, frame), 1U);
        build(frame, version, 1U); frame[56U] = 1U; seal(frame);
        RUN(validate(version, frame), 1U);
        build(frame, version, 1U); put16(frame, 12U, 1U); seal(frame);
        RUN(validate(version, frame), 1U);
    }

    build(frame, 2U, 1U); put64(frame, 24U, 0xA5ULL); seal(frame);
    RUN(edu24_wire_v2_valid(frame), 0U);
    build(frame, 3U, 1U); put64(frame, 24U, 0xA5ULL); seal(frame);
    RUN(edu26_wire_v3_valid(frame), 1U);
    build(frame, 2U, 9U); frame[34U] = 1U; seal(frame);
    RUN(edu24_wire_v2_valid(frame), 1U);

    build(frame, 3U, 4U); put64(frame, 24U, 0ULL); seal(frame);
    RUN(edu26_wire_v3_valid(frame), 1U);
    build(frame, 3U, 4U); put64(frame, 24U, 0x100000000ULL); seal(frame);
    RUN(edu26_wire_v3_valid(frame), 1U);

    build(frame, 4U, 11U); put32(frame, 28U, 0U); seal(frame);
    RUN(edu28_wire_v4_valid(frame), 1U);
    build(frame, 4U, 11U); put32(frame, 28U, 2049U); seal(frame);
    RUN(edu28_wire_v4_valid(frame), 1U);
    build(frame, 4U, 12U); put16(frame, 28U, 128U); seal(frame);
    RUN(edu28_wire_v4_valid(frame), 1U);
    build(frame, 4U, 12U); frame[30U] = 0U; seal(frame);
    RUN(edu28_wire_v4_valid(frame), 1U);
    build(frame, 4U, 12U); frame[30U] = 17U; seal(frame);
    RUN(edu28_wire_v4_valid(frame), 1U);
    build(frame, 4U, 12U); frame[31U] = 1U; seal(frame);
    RUN(edu28_wire_v4_valid(frame), 1U);
    build(frame, 4U, 12U); frame[36U] = 1U; seal(frame);
    RUN(edu28_wire_v4_valid(frame), 1U);
    build(frame, 4U, 15U); put16(frame, 28U, 128U); seal(frame);
    RUN(edu28_wire_v4_valid(frame), 1U);

    build(frame, 5U, 3U);
    RUN(edu29_wire_v5_valid(frame), 0U);
    build(frame, 6U, 3U); put16(frame, 32U, 0U); seal(frame);
    RUN(edu30_wire_v6_valid(frame), 1U);
    build(frame, 6U, 3U); put16(frame, 32U, 5U); seal(frame);
    RUN(edu30_wire_v6_valid(frame), 1U);
    build(frame, 6U, 3U); put16(frame, 32U, 4U); seal(frame);
    RUN(edu30_wire_v6_valid(frame), 0U);
    build(frame, 7U, 17U);
    RUN(edu31_wire_v7_valid(frame), 0U);
    build(frame, 6U, 17U);
    RUN(edu30_wire_v6_valid(frame), 3U);
    RUN(edu31_wire_v7_valid((const u8 *)0), 1U);

    printf(
        "OS-POST-EDU19 edu24-31-wire-history snapshots=06979f3..0d10b3d "
        "vectors=%d digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
