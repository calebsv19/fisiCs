/*
 * Compiler-side contract mirror derived from immutable os-dev tag
 * edu-38-bounded-runner-context-instancing, commit
 * 59d622af0278e0a57a36285ea7b75839a352bc41.
 *
 * Authoritative implementation: queue64.asm
 * SHA-256: 5da3cc8390c9cc94fb59057c523390d725b0b7d9b8efa8609c19419acda0308f.
 * This C mirror probes geometry and active-limit behavior only; it is not OS
 * source and does not own resources, SMP state, persistence, or scheduling.
 */
typedef unsigned char edu38_u8;
typedef unsigned int edu38_u32;
typedef unsigned long long edu38_u64;

enum {
    EDU38_CONTEXT_COUNT = 2,
    EDU38_CONTEXT_BYTES = 160,
    EDU38_CONTEXT_NONE = 2
};

static edu38_u32 edu38_read16(const edu38_u8 *bytes) {
    return (edu38_u32)bytes[0] | ((edu38_u32)bytes[1] << 8);
}

static edu38_u32 edu38_read32(const edu38_u8 *bytes) {
    return (edu38_u32)bytes[0] | ((edu38_u32)bytes[1] << 8) |
           ((edu38_u32)bytes[2] << 16) | ((edu38_u32)bytes[3] << 24);
}

static edu38_u64 edu38_read64(const edu38_u8 *bytes) {
    return (edu38_u64)edu38_read32(bytes) |
           ((edu38_u64)edu38_read32(bytes + 4) << 32);
}

edu38_u64 edu38_runner_context_for_slot(edu38_u64 slot) {
    return slot & (EDU38_CONTEXT_COUNT - 1);
}

/*
 * Mirror the active scan: return lane zero/one, two when idle, or all-ones
 * when both records claim activity. The assembly treats any nonzero active
 * word as active; strict flag shape is checked separately below.
 */
edu38_u64 edu38_runner_find_active(const edu38_u8 *contexts) {
    edu38_u64 found = EDU38_CONTEXT_NONE;
    edu38_u32 lane;
    for (lane = 0; lane < EDU38_CONTEXT_COUNT; lane = lane + 1) {
        const edu38_u8 *record = contexts + lane * EDU38_CONTEXT_BYTES;
        if (edu38_read64(record) != 0) {
            if (found != EDU38_CONTEXT_NONE) return ~0ULL;
            found = lane;
        }
    }
    return found;
}

/*
 * Validate the frozen record shape at a stable turn boundary. Inactive
 * records may retain terminal evidence. Active records must carry the exact
 * identity/resource fields established before the active bit is published.
 */
int edu38_runner_contexts_valid(const edu38_u8 *contexts) {
    edu38_u32 lane;
    edu38_u32 active_count = 0;
    for (lane = 0; lane < EDU38_CONTEXT_COUNT; lane = lane + 1) {
        const edu38_u8 *record = contexts + lane * EDU38_CONTEXT_BYTES;
        edu38_u64 active = edu38_read64(record);
        edu38_u64 checkpoint = edu38_read64(record + 152);
        if (active > 1 || edu38_read64(record + 8) != lane ||
            edu38_read16(record + 62) != 0 ||
            (checkpoint != ~0ULL && checkpoint > 1)) return 0;
        if (active != 0) {
            active_count = active_count + 1;
            if (edu38_read64(record + 16) >= 8 ||
                edu38_read64(record + 24) == 0 ||
                edu38_read64(record + 32) == 0 ||
                edu38_read64(record + 40) == 0 ||
                edu38_read64(record + 48) == 0 ||
                edu38_read32(record + 56) == 0 ||
                edu38_read16(record + 60) != 104 ||
                edu38_read64(record + 64) == 0 ||
                edu38_read64(record + 72) == 0 ||
                edu38_read64(record + 80) == 0 ||
                edu38_read64(record + 88) == 0 ||
                edu38_read64(record + 96) > 5 ||
                edu38_read64(record + 104) > 1 ||
                edu38_read64(record + 144) > 5) return 0;
        }
    }
    return active_count <= 1;
}
