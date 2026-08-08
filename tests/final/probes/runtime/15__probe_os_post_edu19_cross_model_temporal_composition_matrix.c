/* Reuse the established temporal fixture constructors without duplicating them. */
#define main edu45_temporal_fixture_main
#include "15__probe_os_post_edu19_temporal_fault_sequence_matrix.c"
#undef main

extern int edu45_scheduler_handoff_valid(unsigned long, unsigned long);
extern int edu45_cross_model_temporal_admission(
    const u8 *, const u8 *, const u8 *, const u8 *, const u8 *,
    const u8 *, const u8 *, const u8 *, const u8 *, u32, u64);

static void cm_put16(u8 *p, u32 offset, u32 value) {
    p[offset] = (u8)value;
    p[offset + 1U] = (u8)(value >> 8U);
}

static void cm_put32(u8 *p, u32 offset, u32 value) {
    p[offset] = (u8)value;
    p[offset + 1U] = (u8)(value >> 8U);
    p[offset + 2U] = (u8)(value >> 16U);
    p[offset + 3U] = (u8)(value >> 24U);
}

static void cm_put64(u8 *p, u32 offset, u64 value) {
    u32 index;
    for (index = 0U; index < 8U; index = index + 1U) {
        p[offset + index] = (u8)(value >> (index * 8U));
    }
}

static void cm_zero(u8 *p, u32 count) {
    u32 index;
    for (index = 0U; index < count; index = index + 1U) p[index] = 0U;
}

static u32 cm_fnv(const u8 *p, u32 count) {
    u32 value = 2166136261U;
    u32 index;
    for (index = 0U; index < count; index = index + 1U) {
        value = (value ^ p[index]) * 16777619U;
    }
    return value;
}

static void cm_seal(u8 *p, u32 count, u32 checksum_offset) {
    cm_put32(p, checksum_offset, cm_fnv(p, count));
}

static void cm_queue_complete(u8 *p) {
    cm_zero(p, 512U);
    p[0] = 'E'; p[1] = 'D'; p[2] = 'U'; p[3] = '1'; p[4] = '5'; p[5] = 'J';
    cm_put32(p, 8U, 2U); cm_put32(p, 12U, 4U);
    cm_put64(p, 16U, 0xED22000000000001ULL); cm_put32(p, 24U, 3U);
    cm_put32(p, 32U, 2U); cm_put32(p, 36U, 2U);
    cm_put64(p, 40U, 0x6EC4E5DB9E1056CFULL);
    cm_put64(p, 48U, 0x1E3C373BAF48FAF7ULL);
    cm_put16(p, 56U, 1U); cm_put16(p, 58U, 32U); p[60] = 1U; p[61] = 12U;
    cm_put16(p, 104U, 1U); cm_put16(p, 106U, 1U); cm_put16(p, 108U, 11U);
    cm_put64(p, 112U, 100ULL); cm_put32(p, 120U, 3U); cm_put16(p, 488U, 1U);
    cm_put64(p, 64U, 0x6EC4E5DB9E1056CFULL); cm_seal(p, 508U, 508U);
}

static void cm_wire_v7(u8 *p) {
    cm_zero(p, 64U);
    cm_put64(p, 0U, 0x0051523132554445ULL); cm_put16(p, 8U, 7U); p[10] = 17U;
    cm_put16(p, 12U, 0U); cm_put64(p, 16U, 0xED24000000000012ULL);
    cm_seal(p, 60U, 60U);
}

static int cm_checks;
static u32 cm_digest = 2166136261U;

static int cm_expect(u32 id, u64 actual, u64 expected) {
    cm_checks = cm_checks + 1;
    cm_digest = (cm_digest ^ id) * 16777619U;
    cm_digest = (cm_digest ^ (u32)actual) * 16777619U;
    return actual == expected ? 0 : (int)id;
}

int main(void) {
    u8 before[320], restarted[320], old_lanes[1024], new_lanes[1024], entries[4096];
    u8 old_mailbox[112], new_mailbox[112], queue[512], wire[64], candidate[1024];
    int failure;
#define CM_RUN(ID, ACTUAL, EXPECTED) do { \
    failure = cm_expect((ID), (u64)(ACTUAL), (u64)(EXPECTED)); \
    if (failure != 0) return failure; \
} while (0)
    active_01(before);
    copy_bytes(restarted, before, 320U);
    activate(restarted, 0U, 2ULL, 0x1400ULL, 23ULL, 0x201ULL);
    build_lanes(old_lanes, 0U, 21U, 0x101ULL);
    build_lanes(new_lanes, 2U, 23U, 0x201ULL);
    zero_bytes(entries, 4096U);
    build_dispatch_mailbox(old_mailbox, 0U, 0U, 21U, 0x101ULL);
    build_dispatch_mailbox(new_mailbox, 0U, 2U, 23U, 0x201ULL);
    cm_queue_complete(queue); cm_wire_v7(wire);
    CM_RUN(1U, edu45_scheduler_handoff_valid(1UL, 0UL), 1U);
    CM_RUN(2U, edu45_scheduler_handoff_valid(0UL, 1UL), 0U);
    CM_RUN(3U, edu45_cross_model_temporal_admission(
        queue, wire, old_lanes, new_lanes, entries, before, restarted,
        old_mailbox, new_mailbox, 31U, 0x201ULL), 1U);
    queue[508U] ^= 1U;
    CM_RUN(4U, edu45_cross_model_temporal_admission(
        queue, wire, old_lanes, new_lanes, entries, before, restarted,
        old_mailbox, new_mailbox, 31U, 0x201ULL), 0U);
    queue[508U] ^= 1U; wire[60U] ^= 1U;
    CM_RUN(5U, edu45_cross_model_temporal_admission(
        queue, wire, old_lanes, new_lanes, entries, before, restarted,
        old_mailbox, new_mailbox, 31U, 0x201ULL), 0U);
    wire[60U] ^= 1U;
    CM_RUN(6U, edu45_cross_model_temporal_admission(
        queue, wire, old_lanes, new_lanes, entries, before, restarted,
        old_mailbox, new_mailbox, 32U, 0x301ULL), 0U);
    CM_RUN(7U, edu45_cross_model_temporal_admission(
        queue, wire, old_lanes, new_lanes, entries, before, restarted,
        old_mailbox, old_mailbox, 31U, 0x201ULL), 0U);
    copy_bytes(candidate, old_lanes, 1024U);
    CM_RUN(8U, edu45_cross_model_temporal_admission(
        queue, wire, candidate, candidate, entries, before, restarted,
        old_mailbox, new_mailbox, 31U, 0x201ULL), 0U);
    printf("OS-POST-EDU19 cross-model-temporal basis=queue-v2+wire-v7+scheduler vectors=%d digest=%u result=PASS\n", cm_checks, cm_digest);
    return 0;
#undef CM_RUN
}
