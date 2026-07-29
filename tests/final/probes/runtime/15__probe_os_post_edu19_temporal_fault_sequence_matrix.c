typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern int edu44_pre_ack_interruption_blocks_reuse(
    const u8 *before_contexts, const u8 *after_contexts,
    u32 retiring_context, u64 reclaimed_slot,
    u64 reclaimed_entry_address, u32 reclaimed_state,
    u32 reclaimed_generation, u64 reclaimed_request,
    u32 observed_ack_generation, u64 observed_ack_request,
    u32 next_generation, u64 new_request,
    u64 peer_slot, u64 peer_entry_address,
    u64 peer_generation, u64 peer_request);
extern int edu44_post_checkpoint_restart_rejects_stale(
    const u8 *old_lanes, const u8 *new_lanes, const u8 *entries,
    const u8 *before_contexts, const u8 *restarted_contexts,
    u32 context, u32 old_slot, u64 old_entry_address,
    u32 old_generation, u64 old_request, u32 old_lane,
    u32 new_slot, u64 new_entry_address, u32 new_generation,
    u64 new_request, u32 new_lane, u64 peer_slot,
    u64 peer_entry_address, u64 peer_generation, u64 peer_request);
extern int edu44_mid_phase_owner_loss_preserves_peer(
    const u8 *owners, const u8 *before_contexts,
    const u8 *after_contexts, const u8 *lost_mailbox,
    const u8 *peer_mailbox, u32 lost_context, u32 lost_slot,
    u64 lost_entry_address, u32 lost_generation, u64 lost_request,
    u32 lost_workload_generation, u32 lost_width, u32 peer_slot,
    u64 peer_entry_address, u32 peer_generation, u64 peer_request,
    u32 peer_workload_generation, u32 peer_width,
    u32 workload_checksum, const u8 *workload);
extern int edu44_post_completion_retirement_blocks_redispatch(
    const u8 *before_contexts, const u8 *after_contexts,
    const u8 *completed_mailbox, const u8 *retired_mailbox,
    u32 retiring_context, u64 retiring_slot,
    u64 retiring_entry_address, u64 retiring_generation,
    u64 retiring_request, u64 peer_slot, u64 peer_entry_address,
    u64 peer_generation, u64 peer_request,
    u64 work_generation, u64 ap_error);
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

static u32 fnv(const u8 *bytes, u32 count) {
    u32 value = 2166136261U;
    u32 index;
    for (index = 0U; index < count; index = index + 1U) {
        value = (value ^ bytes[index]) * 16777619U;
    }
    return value;
}

static void zero_bytes(u8 *bytes, u32 count) {
    u32 index;
    for (index = 0U; index < count; index = index + 1U) {
        bytes[index] = 0U;
    }
}

static void copy_bytes(u8 *destination, const u8 *source, u32 count) {
    u32 index;
    for (index = 0U; index < count; index = index + 1U) {
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

static void build_lanes(
    u8 *lanes, u32 slot0, u32 generation0, u64 request0) {
    build_snapshot(lanes, slot0, generation0, request0);
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
    for (index = 0U; index < 160U; index = index + 1U) {
        record[index] = 0U;
    }
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

static void build_dispatch_mailbox(
    u8 *mailbox, u32 context, u32 slot,
    u32 generation, u64 request) {
    zero_bytes(mailbox, 112U);
    put64(mailbox, 0U, 1ULL);
    put64(mailbox, 8U, 1ULL);
    put64(mailbox, 16U, 7ULL);
    put64(mailbox, 24U, 6ULL);
    put64(mailbox, 32U, context);
    put64(mailbox, 40U, slot);
    put64(mailbox, 48U, generation);
    put64(mailbox, 56U, request);
    put64(mailbox, 64U, ~0ULL);
    put64(mailbox, 72U, ~0ULL);
}

static void build_completed_mailbox(
    u8 *mailbox, u32 context, u32 slot,
    u32 generation, u64 request, int retired) {
    zero_bytes(mailbox, 112U);
    put64(mailbox, 0U, retired ? 0ULL : 1ULL);
    put64(mailbox, 8U, retired ? 0ULL : 1ULL);
    put64(mailbox, 16U, 7ULL);
    put64(mailbox, 24U, 7ULL);
    put64(mailbox, 32U, context);
    put64(mailbox, 40U, slot);
    put64(mailbox, 48U, generation);
    put64(mailbox, 56U, request);
    put64(mailbox, 64U, context);
    put64(mailbox, 72U, slot);
    put64(mailbox, 80U, generation);
    put64(mailbox, 88U, request);
    put64(mailbox, 96U, 3ULL);
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
    u8 before[320], after[320], candidate_contexts[320];
    u8 old_lanes[1024], new_lanes[1024], candidate_lanes[1024];
    u8 entries[4096], owners[448], workload[104];
    u8 lost_mailbox[112], peer_mailbox[112];
    u8 completed_mailbox[112], retired_mailbox[112], mailbox_candidate[112];
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
    RUN(1U, edu44_pre_ack_interruption_blocks_reuse(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        30U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 1U);
    RUN(2U, edu44_pre_ack_interruption_blocks_reuse(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    copy_bytes(candidate_contexts, after, 320U);
    candidate_contexts[160U + 80U] ^= 1U;
    RUN(3U, edu44_pre_ack_interruption_blocks_reuse(
        before, candidate_contexts, 0U, 4ULL, 0x1800ULL,
        3U, 31U, 0x201ULL, 30U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(4U, edu44_pre_ack_interruption_blocks_reuse(
        before, before, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        30U, 0x201ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 0U);
    RUN(5U, edu44_pre_ack_interruption_blocks_reuse(
        before, after, 0U, 4ULL, 0x1800ULL, 3U, 31U, 0x201ULL,
        31U, 0x200ULL, 32U, 0x301ULL,
        5ULL, 0x1A00ULL, 32ULL, 0x202ULL), 1U);

    active_01(before);
    copy_bytes(after, before, 320U);
    activate(after, 0U, 2ULL, 0x1400ULL, 23ULL, 0x201ULL);
    build_lanes(old_lanes, 0U, 21U, 0x101ULL);
    build_lanes(new_lanes, 2U, 23U, 0x201ULL);
    zero_bytes(entries, 4096U);
    RUN(6U, edu44_post_checkpoint_restart_rejects_stale(
        old_lanes, new_lanes, entries, before, after,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 0U,
        2U, 0x1400ULL, 23U, 0x201ULL, 0U,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL), 1U);
    RUN(7U, edu44_post_checkpoint_restart_rejects_stale(
        old_lanes, old_lanes, entries, before, after,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 0U,
        2U, 0x1400ULL, 23U, 0x201ULL, 0U,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL), 0U);
    copy_bytes(candidate_lanes, new_lanes, 1024U);
    candidate_lanes[508U] ^= 1U;
    RUN(8U, edu44_post_checkpoint_restart_rejects_stale(
        old_lanes, candidate_lanes, entries, before, after,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 0U,
        2U, 0x1400ULL, 23U, 0x201ULL, 0U,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL), 0U);
    copy_bytes(candidate_contexts, after, 320U);
    put64(candidate_contexts + 160U, 32U, 23ULL);
    RUN(9U, edu44_post_checkpoint_restart_rejects_stale(
        old_lanes, new_lanes, entries, before, candidate_contexts,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 0U,
        2U, 0x1400ULL, 23U, 0x201ULL, 0U,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL), 0U);
    RUN(10U, edu44_post_checkpoint_restart_rejects_stale(
        old_lanes, new_lanes, entries, before, after,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 0U,
        2U, 0x1400ULL, 23U, 0x201ULL, 1U,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL), 0U);

    build_workload(workload);
    workload_checksum = fnv(workload, 104U);
    active_01(before);
    copy_bytes(after, before, 320U);
    retire_context(after, 0U);
    build_owners(owners, workload);
    build_dispatch_mailbox(lost_mailbox, 0U, 0U, 21U, 0x101ULL);
    build_dispatch_mailbox(peer_mailbox, 1U, 1U, 22U, 0x102ULL);
    RUN(11U, edu44_mid_phase_owner_loss_preserves_peer(
        owners, before, after, lost_mailbox, peer_mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U, 1U,
        1U, 0x1200ULL, 22U, 0x102ULL, 32U, 2U,
        workload_checksum, workload), 1U);
    RUN(12U, edu44_mid_phase_owner_loss_preserves_peer(
        owners, before, after, lost_mailbox, lost_mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U, 1U,
        1U, 0x1200ULL, 22U, 0x102ULL, 32U, 2U,
        workload_checksum, workload), 0U);
    RUN(13U, edu44_mid_phase_owner_loss_preserves_peer(
        owners, before, before, lost_mailbox, peer_mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U, 1U,
        1U, 0x1200ULL, 22U, 0x102ULL, 32U, 2U,
        workload_checksum, workload), 0U);
    copy_bytes(candidate_contexts, after, 320U);
    candidate_contexts[160U + 96U] ^= 1U;
    RUN(14U, edu44_mid_phase_owner_loss_preserves_peer(
        owners, before, candidate_contexts, lost_mailbox, peer_mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U, 1U,
        1U, 0x1200ULL, 22U, 0x102ULL, 32U, 2U,
        workload_checksum, workload), 0U);
    RUN(15U, edu44_mid_phase_owner_loss_preserves_peer(
        owners, before, after, lost_mailbox, peer_mailbox,
        0U, 0U, 0x1000ULL, 21U, 0x101ULL, 31U, 1U,
        1U, 0x1200ULL, 23U, 0x102ULL, 32U, 2U,
        workload_checksum, workload), 0U);

    active_01(before);
    copy_bytes(after, before, 320U);
    retire_context(after, 0U);
    build_completed_mailbox(
        completed_mailbox, 0U, 0U, 21U, 0x101ULL, 0);
    build_completed_mailbox(
        retired_mailbox, 0U, 0U, 21U, 0x101ULL, 1);
    RUN(16U, edu44_post_completion_retirement_blocks_redispatch(
        before, after, completed_mailbox, retired_mailbox,
        0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL, 7ULL, 0ULL), 1U);
    RUN(17U, edu44_post_completion_retirement_blocks_redispatch(
        before, before, completed_mailbox, retired_mailbox,
        0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL, 7ULL, 0ULL), 0U);
    copy_bytes(mailbox_candidate, retired_mailbox, 112U);
    put64(mailbox_candidate, 0U, 1ULL);
    put64(mailbox_candidate, 8U, 1ULL);
    RUN(18U, edu44_post_completion_retirement_blocks_redispatch(
        before, after, completed_mailbox, mailbox_candidate,
        0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL, 7ULL, 0ULL), 0U);
    copy_bytes(mailbox_candidate, completed_mailbox, 112U);
    put64(mailbox_candidate, 88U, 0x102ULL);
    RUN(19U, edu44_post_completion_retirement_blocks_redispatch(
        before, after, mailbox_candidate, retired_mailbox,
        0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL, 7ULL, 0ULL), 0U);
    copy_bytes(candidate_contexts, after, 320U);
    candidate_contexts[160U + 104U] ^= 1U;
    RUN(20U, edu44_post_completion_retirement_blocks_redispatch(
        before, candidate_contexts, completed_mailbox, retired_mailbox,
        0U, 0ULL, 0x1000ULL, 21ULL, 0x101ULL,
        1ULL, 0x1200ULL, 22ULL, 0x102ULL, 7ULL, 0ULL), 0U);

    printf(
        "OS-POST-EDU19 temporal-fault-sequence "
        "basis=durable-owner-chain-v1 vectors=%d digest=%u result=PASS\n",
        checks, digest);
    return 0;
#undef RUN
}
