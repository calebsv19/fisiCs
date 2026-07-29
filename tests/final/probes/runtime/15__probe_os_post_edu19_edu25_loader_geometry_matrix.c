typedef unsigned long long u64;
typedef unsigned int u32;

extern int edu25_boot_geometry_valid(
    u32 kernel_start_lba, u32 kernel_actual_sectors,
    u32 kernel_max_sectors, u32 filesystem_start_lba,
    u32 kernel_load_address, u32 kernel_entry_address,
    u32 edd_available);
extern u32 edu25_transfer_count(u32 actual_sectors);
extern u32 edu25_transfer_chunk(u32 actual_sectors, u32 transfer_index);
extern u64 edu25_transfer_lba(u32 transfer_index);
extern u64 edu25_transfer_destination(u32 transfer_index);
extern int edu25_transfer_plan_valid(
    u32 actual_sectors, const u32 *chunks, u32 chunk_count);
extern int printf(const char *format, ...);

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
    u32 chunks[4];
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    RUN(1U, edu25_boot_geometry_valid(
        18U, 1U, 256U, 274U, 0x20000U, 0x20000U, 1U), 1U);
    RUN(2U, edu25_boot_geometry_valid(
        18U, 256U, 256U, 274U, 0x20000U, 0x20000U, 1U), 1U);
    RUN(3U, edu25_boot_geometry_valid(
        17U, 1U, 256U, 274U, 0x20000U, 0x20000U, 1U), 0U);
    RUN(4U, edu25_boot_geometry_valid(
        18U, 0U, 256U, 274U, 0x20000U, 0x20000U, 1U), 0U);
    RUN(5U, edu25_boot_geometry_valid(
        18U, 257U, 256U, 274U, 0x20000U, 0x20000U, 1U), 0U);
    RUN(6U, edu25_boot_geometry_valid(
        18U, 1U, 255U, 274U, 0x20000U, 0x20000U, 1U), 0U);
    RUN(7U, edu25_boot_geometry_valid(
        18U, 1U, 256U, 273U, 0x20000U, 0x20000U, 1U), 0U);
    RUN(8U, edu25_boot_geometry_valid(
        18U, 1U, 256U, 274U, 0x1ffffU, 0x20000U, 1U), 0U);
    RUN(9U, edu25_boot_geometry_valid(
        18U, 1U, 256U, 274U, 0x20000U, 0x20001U, 1U), 0U);
    RUN(10U, edu25_boot_geometry_valid(
        18U, 1U, 256U, 274U, 0x20000U, 0x20000U, 0U), 0U);
    RUN(11U, edu25_boot_geometry_valid(
        18U, 1U, 256U, 274U, 0x20000U, 0x20000U, 2U), 0U);

    RUN(12U, edu25_transfer_count(0U), 0U);
    RUN(13U, edu25_transfer_count(1U), 1U);
    RUN(14U, edu25_transfer_count(64U), 1U);
    RUN(15U, edu25_transfer_count(65U), 2U);
    RUN(16U, edu25_transfer_count(128U), 2U);
    RUN(17U, edu25_transfer_count(129U), 3U);
    RUN(18U, edu25_transfer_count(192U), 3U);
    RUN(19U, edu25_transfer_count(193U), 4U);
    RUN(20U, edu25_transfer_count(256U), 4U);
    RUN(21U, edu25_transfer_count(257U), 0U);

    RUN(22U, edu25_transfer_chunk(1U, 0U), 1U);
    RUN(23U, edu25_transfer_chunk(64U, 0U), 64U);
    RUN(24U, edu25_transfer_chunk(64U, 1U), 0U);
    RUN(25U, edu25_transfer_chunk(65U, 0U), 64U);
    RUN(26U, edu25_transfer_chunk(65U, 1U), 1U);
    RUN(27U, edu25_transfer_chunk(128U, 1U), 64U);
    RUN(28U, edu25_transfer_chunk(129U, 2U), 1U);
    RUN(29U, edu25_transfer_chunk(256U, 3U), 64U);
    RUN(30U, edu25_transfer_chunk(257U, 0U), 0U);

    RUN(31U, edu25_transfer_lba(0U), 18U);
    RUN(32U, edu25_transfer_lba(1U), 82U);
    RUN(33U, edu25_transfer_lba(2U), 146U);
    RUN(34U, edu25_transfer_lba(3U), 210U);
    RUN(35U, edu25_transfer_lba(4U), ~0ULL);
    RUN(36U, edu25_transfer_destination(0U), 0x20000U);
    RUN(37U, edu25_transfer_destination(1U), 0x28000U);
    RUN(38U, edu25_transfer_destination(2U), 0x30000U);
    RUN(39U, edu25_transfer_destination(3U), 0x38000U);
    RUN(40U, edu25_transfer_destination(4U), ~0ULL);

    chunks[0] = 64U; chunks[1] = 1U;
    RUN(41U, edu25_transfer_plan_valid(65U, chunks, 2U), 1U);
    chunks[1] = 2U;
    RUN(42U, edu25_transfer_plan_valid(65U, chunks, 2U), 0U);
    chunks[0] = 64U; chunks[1] = 64U;
    chunks[2] = 64U; chunks[3] = 64U;
    RUN(43U, edu25_transfer_plan_valid(256U, chunks, 4U), 1U);
    RUN(44U, edu25_transfer_plan_valid(256U, chunks, 3U), 0U);
    chunks[2] = 63U;
    RUN(45U, edu25_transfer_plan_valid(256U, chunks, 4U), 0U);
    RUN(46U, edu25_transfer_plan_valid(
        256U, (const u32 *)0, 4U), 0U);
    RUN(47U, edu25_transfer_plan_valid(0U, chunks, 0U), 0U);

    printf(
        "OS-POST-EDU19 edu25-loader snapshot=ebed910 vectors=%d "
        "digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
