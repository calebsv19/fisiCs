typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern int edu43_generation_reuse_owner_valid(
    const u8 *before_contexts, const u8 *after_contexts,
    u32 retiring_context, u64 reclaimed_slot,
    u64 reclaimed_entry_address, u32 reclaimed_state,
    u32 reclaimed_generation, u64 reclaimed_request,
    u32 acknowledged_generation, u64 acknowledged_request,
    u32 next_generation, u64 new_request, u64 peer_slot,
    u64 peer_entry_address, u64 peer_generation, u64 peer_request);
extern int edu43_checkpoint_owner_valid(
    const u8 *lanes, const u8 *entries, const u8 *contexts,
    u32 context, u32 slot, u64 entry_address, u32 generation,
    u64 request, u32 expected_lane);
extern int edu43_phase_owner_active_valid(
    const u8 *owners, const u8 *contexts, const u8 *mailbox,
    u32 context, u32 slot, u64 entry_address, u32 queue_generation,
    u64 request, u32 workload_generation, u32 workload_checksum,
    u32 width, const u8 *workload);
extern int edu43_durable_owner_chain_valid(
    const u8 *metadata, u64 entry_lba, const u8 *entry,
    const u8 *lanes, const u8 *entries, const u8 *owners,
    const u8 *contexts, const u8 *mailbox, u32 context, u32 slot,
    u64 entry_address, u32 queue_generation, u64 request,
    u32 workload_generation, u32 workload_checksum, u32 width,
    const u8 *workload, u32 expected_lane, u64 work_generation);
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
    for (index = 0; index < 8U; index = index + 1U) {
        bytes[offset + index] = (u8)(value >> (index * 8U));
    }
}

static u32 fnv(const u8 *bytes, u32 count) {
    u32 value = 2166136261U;
    u32 index;
    for (index = 0; index < count; index = index + 1U) {
        value = (value ^ bytes[index]) * 16777619U;
    }
    return value;
}

static void zero_bytes(u8 *bytes, u32 count) {
    u32 index;
    for (index = 0; index < count; index = index + 1U) bytes[index] = 0U;
}

static void copy_bytes(u8 *destination, const u8 *source, u32 count) {
    u32 index;
    for (index = 0; index < count; index = index + 1U) {
        destination[index] = source[index];
    }
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

static void finish_snapshot(u8 *bytes) {
    put32(bytes, 40U, fnv(bytes + 80U, 104U));
    put32(bytes, 232U, fnv(bytes + 80U, 152U));
    put32(bytes, 236U, fnv(bytes, 236U));
    put32(bytes, 508U, fnv(bytes, 508U));
}

static void build_snapshot(
    u8 *bytes, u32 slot, u32 generation, u64 request) {
    u32 index;
    zero_bytes(bytes, 512U);
    bytes[0] = 'E'; bytes[1] = 'D'; bytes[2] = 'U'; bytes[3] = '3';
    bytes[4] = '5'; bytes[5] = 'C'; bytes[6] = 'P';
    put16(bytes, 8U, 1U);
    put16(bytes, 10U, 1U);
    put32(bytes, 12U, 240U);
    put32(bytes, 16U, slot);
    put32(bytes, 20U, generation);
    put64(bytes, 24U, request);
    put32(bytes, 32U, 11U);
    put16(bytes, 36U, 104U);
    put16(bytes, 38U, 3U);
    put16(bytes, 44U, 152U);
    put16(bytes, 46U, 3U);
    put64(bytes, 48U, 0x3156504D49534445ULL);
    put64(bytes, 56U, 0x1E3C373BAF48FAF7ULL);
    put64(bytes, 64U, 1000ULL);
    put16(bytes, 72U, 4U);
    put16(bytes, 74U, 4U);
    put16(bytes, 76U, 1U);
    build_workload(bytes + 80U);
    for (index = 0U; index < 3U; index = index + 1U) {
        put64(bytes, 184U + index * 8U, 0x4054800000000000ULL);
        put64(bytes, 208U + index * 8U, 0x403A800000000000ULL);
    }
    finish_snapshot(bytes);
}

static void build_lanes(u8 *lanes) {
    build_snapshot(lanes, 0U, 21U, 0x101ULL);
    build_snapshot(lanes + 512U, 1U, 22U, 0x102ULL);
}

static void init_contexts(u8 *contexts) {
    zero_bytes(contexts, 320U);
    put64(contexts, 8U, 0ULL);
    put64(contexts, 16U, ~0ULL);
    put64(contexts, 152U, ~0ULL);
    put64(contexts + 160U, 8U, 1ULL);
    put64(contexts + 160U, 16U, ~0ULL);
    put64(contexts + 160U, 152U, ~0ULL);
}

static void activate(
    u8 *contexts, u32 context, u64 slot, u64 entry_address,
    u64 generation, u64 request) {
    u8 *record = contexts + context * 160U;
    put64(record, 0U, 1ULL);
    put64(record, 8U, context);
    put64(record, 16U, slot);
    put64(record, 24U, entry_address);
    put64(record, 32U, generation);
    put64(record, 40U, request);
}

static void active_01(u8 *contexts) {
    init_contexts(contexts);
    activate(contexts, 0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL);
    activate(contexts, 1U, 1ULL, 0x1200ULL, 22ULL, 0x102ULL);
}

static void active_45(u8 *contexts) {
    init_contexts(contexts);
    activate(contexts, 0U, 4ULL, 0x1800ULL, 31ULL, 0x201ULL);
    activate(contexts, 1U, 5ULL, 0x1A00ULL, 32ULL, 0x202ULL);
}

static void retire_context(u8 *contexts, u32 context) {
    u32 index;
    u8 *record = contexts + context * 160U;
    for (index = 0; index < 160U; index = index + 1U) record[index] = 0U;
    put64(record, 8U, context);
    put64(record, 16U, ~0ULL);
    put64(record, 152U, ~0ULL);
}

static void build_owner(
    u8 *record, u32 context, u32 slot, u32 queue_generation,
    u64 request, u32 workload_generation, u32 width,
    const u8 *workload) {
    zero_bytes(record, 224U);
    record[0] = 1U;
    record[1] = (u8)context;
    record[2] = 3U;
    put32(record, 4U, slot);
    put32(record, 8U, queue_generation);
    put32(record, 12U, workload_generation);
    put64(record, 16U, request);
    put16(record, 24U, 104U);
    put16(record, 26U, (u16)width);
    put32(record, 28U, fnv(workload, 104U));
    copy_bytes(record + 32U, workload, 104U);
    put64(record, 136U, 0x3900000000000001ULL + context);
    put64(record, 144U, 0x3900000000000002ULL + context);
    put64(record, 152U, 0x3900000000000003ULL + context);
    put64(record, 160U, 0x3910000000000001ULL + context);
    put64(record, 168U, 0x3910000000000002ULL + context);
    put64(record, 176U, 0x3910000000000003ULL + context);
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

static void build_owners(u8 *owners, const u8 *workload) {
    build_owner(
        owners, 0U, 0U, 21U, 0x101ULL, 31U, 1U, workload);
    build_owner(
        owners + 224U, 1U, 1U, 22U, 0x102ULL, 32U, 2U, workload);
}

static void build_mailbox(
    u8 *mailbox, u32 context, u32 slot,
    u32 queue_generation, u64 request) {
    zero_bytes(mailbox, 112U);
    put64(mailbox, 0U, 1ULL);
    put64(mailbox, 8U, 1ULL);
    put64(mailbox, 16U, 7ULL);
    put64(mailbox, 24U, 6ULL);
    put64(mailbox, 32U, context);
    put64(mailbox, 40U, slot);
    put64(mailbox, 48U, queue_generation);
    put64(mailbox, 56U, request);
    put64(mailbox, 64U, ~0ULL);
    put64(mailbox, 72U, ~0ULL);
}

static void build_metadata(u8 *metadata) {
    u32 slot;
    zero_bytes(metadata, 512U);
    metadata[0] = 'E'; metadata[1] = 'D'; metadata[2] = 'U';
    metadata[3] = '1'; metadata[4] = '5'; metadata[5] = 'Q';
    put32(metadata, 8U, 5U);
    put32(metadata, 12U, 8U);
    put32(metadata, 16U, 120U);
    put32(metadata, 24U, 3U);
    put32(metadata, 28U, 512U);
    put32(metadata, 32U, 1U);
    put32(metadata, 36U, 32U);
    put32(metadata, 40U, 12U);
    for (slot = 0; slot < 8U; slot = slot + 1U) {
        put32(metadata, 44U + slot * 4U, 1U);
    }
    put32(metadata, 44U, 22U);
    put32(metadata, 48U, 23U);
    put32(metadata, 508U, fnv(metadata, 508U));
}

static void build_entry(
    u8 *entry, u32 generation, u64 request, u32 state) {
    zero_bytes(entry, 512U);
    put32(entry, 12U, generation);
    put64(entry, 16U, request);
    put32(entry, 24U, state);
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
    u8 before[320], after[320], contexts[320], context_candidate[320];
    u8 lanes[1024], lane_candidate[1024], entries[4096];
    u8 workload[104], workload_candidate[104], owners[448], owner_candidate[448];
    u8 mailbox[112], mailbox_candidate[112];
    u8 metadata[512], metadata_candidate[512], entry[512], entry_candidate[512];
    u32 workload_checksum;
    int failure;

#define RUN(ID, ACTUAL, EXPECTED) \
    do { \
        failure = expect_case((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
        if (failure != 0) return failure; \
    } while (0)

    active_45(before);
    copy_bytes(after, before, 320U);
    retire_context(after, 0U);
    RUN(1U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 1U);
    RUN(2U, edu43_generation_reuse_owner_valid(
        before, after, 1U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(3U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 5ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(4U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1801ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(5U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1800ULL, 2U, 31U, 0x201ULL,
        31U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(6U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        30U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(7U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x200ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(8U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 31U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(9U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 0xFFFFFFFFU, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(10U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 32U, 0x201ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    copy_bytes(context_candidate, after, 320U);
    context_candidate[160U + 80U] ^= 1U;
    RUN(11U, edu43_generation_reuse_owner_valid(
        before, context_candidate, 0U, 4ULL, 0x1800ULL,
        3U, 31U, 0x201ULL, 31U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(12U, edu43_generation_reuse_owner_valid(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 33ULL, 0x202ULL), 0U);

    build_workload(workload);
    workload_checksum = fnv(workload, 104U);
    active_01(contexts);
    build_lanes(lanes);
    zero_bytes(entries, 4096U);
    RUN(13U, edu43_checkpoint_owner_valid(
        lanes, entries, contexts, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 0U), 1U);
    RUN(14U, edu43_checkpoint_owner_valid(
        lanes, entries, contexts, 1U, 1U, 0x1200ULL,
        22U, 0x102ULL, 1U), 1U);
    RUN(15U, edu43_checkpoint_owner_valid(
        lanes, entries, contexts, 1U, 0U, 0x1000ULL,
        21U, 0x101ULL, 0U), 0U);
    RUN(16U, edu43_checkpoint_owner_valid(
        lanes, entries, contexts, 0U, 0U, 0x1001ULL,
        21U, 0x101ULL, 0U), 0U);
    RUN(17U, edu43_checkpoint_owner_valid(
        lanes, entries, contexts, 0U, 0U, 0x1000ULL,
        22U, 0x101ULL, 0U), 0U);
    RUN(18U, edu43_checkpoint_owner_valid(
        lanes, entries, contexts, 0U, 0U, 0x1000ULL,
        21U, 0x102ULL, 0U), 0U);
    RUN(19U, edu43_checkpoint_owner_valid(
        lanes, entries, contexts, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 1U), 0U);
    copy_bytes(lane_candidate, lanes, 1024U);
    lane_candidate[508U] ^= 1U;
    RUN(20U, edu43_checkpoint_owner_valid(
        lane_candidate, entries, contexts, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 0U), 0U);
    copy_bytes(lane_candidate, lanes, 1024U);
    build_snapshot(lane_candidate + 512U, 0U, 21U, 0x101ULL);
    RUN(21U, edu43_checkpoint_owner_valid(
        lane_candidate, entries, contexts, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 0U), 0U);
    copy_bytes(context_candidate, contexts, 320U);
    put64(context_candidate, 0U, 0ULL);
    RUN(22U, edu43_checkpoint_owner_valid(
        lanes, entries, context_candidate, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 0U), 0U);
    RUN(23U, edu43_checkpoint_owner_valid(
        (const u8 *)0, entries, contexts, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 0U), 0U);

    build_owners(owners, workload);
    build_mailbox(mailbox, 0U, 0U, 21U, 0x101ULL);
    RUN(24U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 31U, workload_checksum, 1U, workload), 1U);
    build_mailbox(mailbox, 1U, 1U, 22U, 0x102ULL);
    RUN(25U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 1U, 1U, 0x1200ULL,
        22U, 0x102ULL, 32U, workload_checksum, 2U, workload), 1U);
    RUN(26U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 0U, 1U, 0x1200ULL,
        22U, 0x102ULL, 32U, workload_checksum, 2U, workload), 0U);
    RUN(27U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 1U, 1U, 0x1201ULL,
        22U, 0x102ULL, 32U, workload_checksum, 2U, workload), 0U);
    RUN(28U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 1U, 1U, 0x1200ULL,
        23U, 0x102ULL, 32U, workload_checksum, 2U, workload), 0U);
    RUN(29U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 1U, 1U, 0x1200ULL,
        22U, 0x103ULL, 32U, workload_checksum, 2U, workload), 0U);
    RUN(30U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 1U, 1U, 0x1200ULL,
        22U, 0x102ULL, 31U, workload_checksum, 2U, workload), 0U);
    RUN(31U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 1U, 1U, 0x1200ULL,
        22U, 0x102ULL, 32U, workload_checksum + 1U, 2U, workload), 0U);
    copy_bytes(workload_candidate, workload, 104U);
    workload_candidate[0] ^= 1U;
    RUN(32U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox, 1U, 1U, 0x1200ULL,
        22U, 0x102ULL, 32U, workload_checksum, 2U,
        workload_candidate), 0U);
    copy_bytes(owner_candidate, owners, 448U);
    owner_candidate[224U + 28U] ^= 1U;
    RUN(33U, edu43_phase_owner_active_valid(
        owner_candidate, contexts, mailbox, 1U, 1U, 0x1200ULL,
        22U, 0x102ULL, 32U, workload_checksum, 2U, workload), 0U);
    copy_bytes(mailbox_candidate, mailbox, 112U);
    put64(mailbox_candidate, 32U, 0ULL);
    RUN(34U, edu43_phase_owner_active_valid(
        owners, contexts, mailbox_candidate, 1U, 1U, 0x1200ULL,
        22U, 0x102ULL, 32U, workload_checksum, 2U, workload), 0U);

    build_metadata(metadata);
    build_entry(entry, 21U, 0x101ULL, 2U);
    build_mailbox(mailbox, 0U, 0U, 21U, 0x101ULL);
    RUN(35U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry, lanes, entries, owners, contexts, mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U,
        workload_checksum, 1U, workload, 0U, 7ULL), 1U);
    copy_bytes(metadata_candidate, metadata, 512U);
    metadata_candidate[508U] ^= 1U;
    RUN(36U, edu43_durable_owner_chain_valid(
        metadata_candidate, 120ULL, entry, lanes, entries,
        owners, contexts, mailbox, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 31U, workload_checksum, 1U,
        workload, 0U, 7ULL), 0U);
    RUN(37U, edu43_durable_owner_chain_valid(
        metadata, 121ULL, entry, lanes, entries, owners, contexts, mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U,
        workload_checksum, 1U, workload, 0U, 7ULL), 0U);
    copy_bytes(entry_candidate, entry, 512U);
    put32(entry_candidate, 12U, 22U);
    RUN(38U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry_candidate, lanes, entries,
        owners, contexts, mailbox, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 31U, workload_checksum, 1U,
        workload, 0U, 7ULL), 0U);
    copy_bytes(entry_candidate, entry, 512U);
    put64(entry_candidate, 16U, 0x102ULL);
    RUN(39U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry_candidate, lanes, entries,
        owners, contexts, mailbox, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 31U, workload_checksum, 1U,
        workload, 0U, 7ULL), 1U);
    copy_bytes(lane_candidate, lanes, 1024U);
    lane_candidate[236U] ^= 1U;
    put32(lane_candidate, 508U, fnv(lane_candidate, 508U));
    RUN(40U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry, lane_candidate, entries,
        owners, contexts, mailbox, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 31U, workload_checksum, 1U,
        workload, 0U, 7ULL), 0U);
    copy_bytes(owner_candidate, owners, 448U);
    put32(owner_candidate, 8U, 22U);
    RUN(41U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry, lanes, entries,
        owner_candidate, contexts, mailbox, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 31U, workload_checksum, 1U,
        workload, 0U, 7ULL), 0U);
    copy_bytes(context_candidate, contexts, 320U);
    put64(context_candidate, 32U, 22ULL);
    RUN(42U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry, lanes, entries,
        owners, context_candidate, mailbox, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 31U, workload_checksum, 1U,
        workload, 0U, 7ULL), 0U);
    copy_bytes(mailbox_candidate, mailbox, 112U);
    put64(mailbox_candidate, 56U, 0x102ULL);
    RUN(43U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry, lanes, entries,
        owners, contexts, mailbox_candidate, 0U, 0U, 0x1000ULL,
        21U, 0x101ULL, 31U, workload_checksum, 1U,
        workload, 0U, 7ULL), 0U);
    RUN(44U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry, lanes, entries, owners, contexts, mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U,
        workload_checksum, 1U, workload, 1U, 7ULL), 0U);
    RUN(45U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry, lanes, entries, owners, contexts, mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U,
        workload_checksum, 1U, workload, 0U, 8ULL), 0U);
    RUN(46U, edu43_durable_owner_chain_valid(
        metadata, 120ULL, entry, lanes, entries, owners, contexts, mailbox,
        1U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U,
        workload_checksum, 1U, workload, 0U, 7ULL), 0U);

    printf(
        "OS-POST-EDU19 durable-owner-chain "
        "snapshots=8359429+5b39037+30e34df+6dd5cd2+1efb1ac+695ec66 "
        "vectors=%d digest=%u result=PASS\n",
        checks,
        digest);
    return 0;
#undef RUN
}
