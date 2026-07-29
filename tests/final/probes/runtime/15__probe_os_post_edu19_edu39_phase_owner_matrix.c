typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern u64 edu39_phase_owner_for_context(u64 context_id);
extern int edu39_phase_owner_record_valid(const u8 *record, u32 expected_id);
extern int edu39_phase_owner_pair_valid(const u8 *owners);
extern int edu39_phase_owner_loadable(const u8 *record, u32 expected_id);
extern int edu39_phase_owner_matches(
    const u8 *record, u32 context_id, u32 slot, u32 queue_generation,
    u64 request_id, u32 workload_generation, u32 workload_length,
    u32 workload_checksum, u32 width, const u8 *workload);
extern int edu39_phase_switch_allowed(
    u64 current_context, u32 prospective_context, u32 phase, u32 width,
    u64 work_done);
extern int edu39_phase_inflight(
    u64 current_context, u32 phase, u32 width, u32 work_mode, u64 work_done);
extern int edu39_phase_publication_path_class(const u8 *record);
extern int printf(const char *format, ...);

static void put16(u8 *bytes, u32 offset, u16 value) {
    bytes[offset] = (u8)value;
    bytes[offset + 1U] = (u8)(value >> 8U);
}

static void put32(u8 *bytes, u32 offset, u32 value) {
    bytes[offset] = (u8)value;
    bytes[offset + 1U] = (u8)(value >> 8U);
    bytes[offset + 2U] = (u8)(value >> 16U);
    bytes[offset + 3U] = (u8)(value >> 24U);
}

static void put64(u8 *bytes, u32 offset, u64 value) {
    u32 index;
    for (index = 0U; index < 8U; index = index + 1U) {
        bytes[offset + index] = (u8)(value >> (index * 8U));
    }
}

static void zero_bytes(u8 *bytes, u32 count) {
    u32 index;
    for (index = 0U; index < count; index = index + 1U) bytes[index] = 0U;
}

static void copy_bytes(u8 *destination, const u8 *source, u32 count) {
    u32 index;
    for (index = 0U; index < count; index = index + 1U) {
        destination[index] = source[index];
    }
}

static u32 fnv(const u8 *bytes, u32 count) {
    u32 value = 0x811C9DC5U;
    u32 index;
    for (index = 0U; index < count; index = index + 1U) {
        value = (value ^ bytes[index]) * 0x01000193U;
    }
    return value;
}

static void build_workload(u8 *bytes) {
    zero_bytes(bytes, 104U);
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '3';
    bytes[4] = '2'; bytes[5] = 'W'; bytes[6] = '1';
    put16(bytes, 8U, 1U);
    put16(bytes, 10U, 1U);
    put32(bytes, 12U, 104U);
    put64(bytes, 16U, 0x3FF0000000000000ULL);
    put64(bytes, 24U, 0x3FD0000000000000ULL);
    put64(bytes, 32U, 0x3FA0000000000000ULL);
    put64(bytes, 40U, 0x4000000000000000ULL);
    put64(bytes, 48U, 0xBFC0000000000000ULL);
    put64(bytes, 56U, 0x3F90000000000000ULL);
    put64(bytes, 64U, 64ULL);
    put64(bytes, 72U, 0x9E3779B97F4A7C15ULL);
    put64(bytes, 80U, 0x4054800000000000ULL);
    put64(bytes, 88U, 0x403A800000000000ULL);
    put64(bytes, 96U, 0x6EC4E5DB9E1056CFULL);
}

static void init_owners(u8 *owners) {
    zero_bytes(owners, 448U);
    owners[1] = 0U;
    owners[224U + 1U] = 1U;
}

static void build_owner(u8 *record, u32 context_id, u32 width, u32 phase) {
    u8 workload[104];
    zero_bytes(record, 224U);
    build_workload(workload);
    record[0] = 1U;
    record[1] = (u8)context_id;
    record[2] = (u8)phase;
    record[3] = phase == 4U ? 1U : 0U;
    put32(record, 4U, context_id);
    put32(record, 8U, 11U + context_id);
    put32(record, 12U, 21U + context_id);
    put64(record, 16U, 0xED39000000000001ULL + context_id);
    put16(record, 24U, 104U);
    put16(record, 26U, (u16)width);
    put32(record, 28U, fnv(workload, 104U));
    copy_bytes(record + 32U, workload, 104U);
    put64(record, 136U, 0x3900000000000001ULL + context_id);
    put64(record, 144U, 0x3900000000000002ULL + context_id);
    put64(record, 152U, 0x3900000000000003ULL + context_id);
    put64(record, 160U, 0x3910000000000001ULL + context_id);
    put64(record, 168U, 0x3910000000000002ULL + context_id);
    put64(record, 176U, 0x3910000000000003ULL + context_id);
    put64(record, 184U, 0x6EC4E5DB9E1056CFULL);
    if (width == 1U) {
        put64(record, 192U, 6ULL);
    } else {
        put64(record, 192U, 3ULL);
        put64(record, 200U, 1ULL);
        put64(record, 208U, 1ULL);
        put64(record, 216U, 3ULL);
    }
}

static int checks;
static u32 digest = 2166136261U;

static int expect_case(u32 id, u64 actual, u64 expected) {
    checks = checks + 1;
    digest = (digest ^ id) * 16777619U;
    digest = (digest ^ (u32)actual) * 16777619U;
    digest = (digest ^ (u32)(actual >> 32U)) * 16777619U;
    if (actual != expected) return (int)id;
    return 0;
}

int main(void) {
    u8 owners[448];
    u8 candidate[448];
    u8 workload[104];
    u8 other_workload[104];
    u8 unaligned[449];
    u32 checksum;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    build_workload(workload);
    checksum = fnv(workload, 104U);
    init_owners(owners);
    RUN(1U, edu39_phase_owner_pair_valid(owners), 1U);
    RUN(2U, edu39_phase_owner_record_valid(owners, 0U), 1U);
    RUN(3U, edu39_phase_owner_record_valid(owners + 224U, 1U), 1U);
    RUN(4U, edu39_phase_owner_for_context(0ULL), 0ULL);
    RUN(5U, edu39_phase_owner_for_context(1ULL), 1ULL);
    RUN(6U, edu39_phase_owner_for_context(2ULL), ~0ULL);
    RUN(7U, edu39_phase_owner_for_context(~0ULL), ~0ULL);

    build_owner(owners, 0U, 1U, 3U);
    build_owner(owners + 224U, 1U, 2U, 3U);
    RUN(8U, edu39_phase_owner_pair_valid(owners), 1U);
    copy_bytes(candidate, owners, 448U); candidate[0] = 2U;
    RUN(9U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); candidate[225U] = 0U;
    RUN(10U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); candidate[1U] = 1U;
    RUN(11U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); candidate[2U] = 5U;
    RUN(12U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); candidate[3U] = 2U;
    RUN(13U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); candidate[2U] = 4U;
    RUN(14U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); candidate[3U] = 1U;
    RUN(15U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); put32(candidate, 4U, 8U);
    RUN(16U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); put32(candidate, 8U, 0U);
    RUN(17U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); put32(candidate, 12U, 0U);
    RUN(18U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); put64(candidate, 16U, 0ULL);
    RUN(19U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); put16(candidate, 24U, 103U);
    RUN(20U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); put16(candidate, 26U, 0U);
    RUN(21U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); put16(candidate, 26U, 3U);
    RUN(22U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); candidate[28U] ^= 1U;
    RUN(23U, edu39_phase_owner_pair_valid(candidate), 0U);
    copy_bytes(candidate, owners, 448U); candidate[32U] = 'X';
    put32(candidate, 28U, fnv(candidate + 32U, 104U));
    RUN(24U, edu39_phase_owner_pair_valid(candidate), 0U);
    RUN(25U, edu39_phase_owner_record_valid((const u8 *)0, 0U), 0U);
    RUN(26U, edu39_phase_owner_record_valid(owners, 2U), 0U);

    init_owners(candidate);
    put64(candidate, 16U, 0xDEADBEEFULL);
    candidate[2U] = 0xFFU;
    RUN(27U, edu39_phase_owner_record_valid(candidate, 0U), 1U);
    RUN(28U, edu39_phase_owner_pair_valid(candidate), 1U);

    build_owner(candidate, 0U, 1U, 2U);
    RUN(29U, edu39_phase_owner_loadable(candidate, 0U), 1U);
    build_owner(candidate, 0U, 2U, 2U);
    RUN(30U, edu39_phase_owner_loadable(candidate, 0U), 0U);
    build_owner(candidate, 0U, 2U, 3U);
    RUN(31U, edu39_phase_owner_loadable(candidate, 0U), 1U);
    build_owner(candidate, 0U, 2U, 4U);
    RUN(32U, edu39_phase_owner_loadable(candidate, 0U), 1U);
    candidate[0U] = 0U;
    RUN(33U, edu39_phase_owner_loadable(candidate, 0U), 0U);

    build_owner(candidate, 0U, 1U, 3U);
    RUN(34U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 11U, 0xED39000000000001ULL,
        21U, 104U, checksum, 1U, workload), 1U);
    RUN(35U, edu39_phase_owner_matches(
        candidate, 1U, 0U, 11U, 0xED39000000000001ULL,
        21U, 104U, checksum, 1U, workload), 0U);
    RUN(36U, edu39_phase_owner_matches(
        candidate, 0U, 1U, 11U, 0xED39000000000001ULL,
        21U, 104U, checksum, 1U, workload), 0U);
    RUN(37U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 12U, 0xED39000000000001ULL,
        21U, 104U, checksum, 1U, workload), 0U);
    RUN(38U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 11U, 0xED39000000000002ULL,
        21U, 104U, checksum, 1U, workload), 0U);
    RUN(39U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 11U, 0xED39000000000001ULL,
        22U, 104U, checksum, 1U, workload), 0U);
    RUN(40U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 11U, 0xED39000000000001ULL,
        21U, 103U, checksum, 1U, workload), 0U);
    RUN(41U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 11U, 0xED39000000000001ULL,
        21U, 104U, checksum + 1U, 1U, workload), 0U);
    RUN(42U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 11U, 0xED39000000000001ULL,
        21U, 104U, checksum, 2U, workload), 0U);
    copy_bytes(other_workload, workload, 104U); other_workload[0U] = 'X';
    RUN(43U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 11U, 0xED39000000000001ULL,
        21U, 104U, checksum, 1U, other_workload), 0U);
    RUN(44U, edu39_phase_owner_matches(
        candidate, 0U, 0U, 11U, 0xED39000000000001ULL,
        21U, 104U, checksum, 1U, (const u8 *)0), 0U);

    RUN(45U, edu39_phase_switch_allowed(~0ULL, 0U, 2U, 2U, 0ULL), 1U);
    RUN(46U, edu39_phase_switch_allowed(0ULL, 0U, 2U, 2U, 0ULL), 1U);
    RUN(47U, edu39_phase_switch_allowed(0ULL, 1U, 1U, 2U, 0ULL), 1U);
    RUN(48U, edu39_phase_switch_allowed(0ULL, 1U, 2U, 1U, 0ULL), 1U);
    RUN(49U, edu39_phase_switch_allowed(0ULL, 1U, 2U, 2U, 0ULL), 0U);
    RUN(50U, edu39_phase_switch_allowed(0ULL, 1U, 2U, 2U, 1ULL), 0U);
    RUN(51U, edu39_phase_switch_allowed(2ULL, 1U, 1U, 1U, 0ULL), 0U);
    RUN(52U, edu39_phase_switch_allowed(0ULL, 2U, 1U, 1U, 0ULL), 0U);

    RUN(53U, edu39_phase_inflight(0ULL, 2U, 2U, 2U, 0ULL), 1U);
    RUN(54U, edu39_phase_inflight(0ULL, 2U, 2U, 2U, 1ULL), 0U);
    RUN(55U, edu39_phase_inflight(0ULL, 2U, 2U, 0U, 0ULL), 0U);
    RUN(56U, edu39_phase_inflight(0ULL, 3U, 2U, 2U, 0ULL), 0U);
    RUN(57U, edu39_phase_inflight(0ULL, 2U, 1U, 2U, 0ULL), 0U);
    RUN(58U, edu39_phase_inflight(~0ULL, 2U, 2U, 2U, 0ULL), 0U);

    build_owner(candidate, 0U, 1U, 3U);
    RUN(59U, edu39_phase_publication_path_class(candidate), 1U);
    build_owner(candidate, 0U, 2U, 3U);
    RUN(60U, edu39_phase_publication_path_class(candidate), 2U);
    put64(candidate, 192U, 0ULL); put64(candidate, 200U, 0ULL);
    put64(candidate, 208U, 0ULL); put64(candidate, 216U, 0ULL);
    RUN(61U, edu39_phase_publication_path_class(candidate), 3U);
    put64(candidate, 216U, 1ULL);
    RUN(62U, edu39_phase_publication_path_class(candidate), 0U);
    build_owner(candidate, 0U, 1U, 2U);
    RUN(63U, edu39_phase_publication_path_class(candidate), 0U);

    build_owner(owners, 0U, 1U, 3U);
    build_owner(owners + 224U, 1U, 2U, 3U);
    put64(owners, 136U, 0xFFFFFFFFFFFFFFFFULL);
    put64(owners + 224U, 184U, 0ULL);
    RUN(64U, edu39_phase_owner_pair_valid(owners), 1U);
    unaligned[0U] = 0xA5U;
    copy_bytes(unaligned + 1U, owners, 448U);
    RUN(65U, edu39_phase_owner_pair_valid(unaligned + 1U), 1U);
    RUN(66U, edu39_phase_owner_record_valid(unaligned + 1U, 0U), 1U);
    RUN(67U, edu39_phase_owner_matches(
        unaligned + 1U, 0U, 0U, 11U, 0xED39000000000001ULL,
        21U, 104U, checksum, 1U, workload), 1U);
    RUN(68U, edu39_phase_owner_record_valid(owners + 224U, 1U), 1U);
    build_owner(candidate, 0U, 2U, 0U);
    RUN(69U, edu39_phase_owner_loadable(candidate, 0U), 1U);
    build_owner(candidate, 0U, 2U, 4U);
    RUN(70U, edu39_phase_owner_loadable(candidate, 0U), 1U);

    printf(
        "OS-POST-EDU19 edu39-phase-owner snapshot=6dd5cd2 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
