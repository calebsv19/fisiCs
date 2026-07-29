/*
 * Compiler-side contract mirror derived from immutable os-dev tag
 * edu-26-generation-safe-queue-reuse, commit
 * 8359429a6b9c9502d48cb7ffd99b2a96b059ef0a.
 *
 * Exact queue_kernel.c SHA-256:
 * 31a79cf4838b5b80999520ee2e1991e078eb7dd010fba9e70456ddbc095e9c88
 *
 * This mirror retains metadata-v5 generation/tombstone validation and the
 * exact entry-generation action. Storage ordering and slot mutation remain
 * assembly-owned.
 */
typedef unsigned char edu26_u8;
typedef unsigned int edu26_u32;
typedef unsigned long long edu26_u64;

enum {
    EDU26_SECTOR = 512,
    EDU26_ENTRY_COUNT = 8,
    EDU26_META_VERSION = 5,
    EDU26_ENTRY_VERSION = 3,
    EDU26_TRACE_VERSION = 1,
    EDU26_TRACE_EVENT_BYTES = 32,
    EDU26_TRACE_CAPACITY = 12,
    EDU26_META_NEXT_GENERATIONS = 44,
    EDU26_META_ACK_GENERATIONS = 76,
    EDU26_META_ACK_REQUEST_IDS = 108,
    EDU26_META_RESERVED = 172,
    EDU26_STATE_COMPLETE = 3,
    EDU26_STATE_FAILED = 4,
    EDU26_STATE_CANCELLED = 5,
    EDU26_OK = 0,
    EDU26_ERR_FORMAT = 1,
    EDU26_ACK_PREPARED = 4
};

static edu26_u32 edu26_read32(const edu26_u8 *p) {
    return (edu26_u32)p[0] | ((edu26_u32)p[1] << 8) |
           ((edu26_u32)p[2] << 16) | ((edu26_u32)p[3] << 24);
}

static edu26_u64 edu26_read64(const edu26_u8 *p) {
    return (edu26_u64)edu26_read32(p) |
           ((edu26_u64)edu26_read32(p + 4) << 32);
}

static edu26_u32 edu26_fnv(const edu26_u8 *p, edu26_u64 count) {
    edu26_u32 hash = 2166136261U;
    edu26_u64 index;
    for (index = 0; index < count; index = index + 1) {
        hash = (hash ^ p[index]) * 16777619U;
    }
    return hash;
}

int edu26_queue_meta_valid(
    const edu26_u8 *p, edu26_u64 entry_lba, edu26_u32 entry_count) {
    edu26_u64 index;
    if (p == (const edu26_u8 *)0 ||
        entry_count != EDU26_ENTRY_COUNT ||
        edu26_read64(p) != 0x0000513531554445ULL ||
        edu26_read32(p + 8) != EDU26_META_VERSION ||
        edu26_read32(p + 12) != entry_count ||
        edu26_read32(p + 16) != entry_lba ||
        edu26_read32(p + 20) != 0 ||
        edu26_read32(p + 24) != EDU26_ENTRY_VERSION ||
        edu26_read32(p + 28) != EDU26_SECTOR ||
        edu26_read32(p + 32) != EDU26_TRACE_VERSION ||
        edu26_read32(p + 36) != EDU26_TRACE_EVENT_BYTES ||
        edu26_read32(p + 40) != EDU26_TRACE_CAPACITY) return 0;
    for (index = 0; index < entry_count; index = index + 1) {
        edu26_u32 next_generation =
            edu26_read32(p + EDU26_META_NEXT_GENERATIONS + index * 4);
        edu26_u32 acknowledged_generation =
            edu26_read32(p + EDU26_META_ACK_GENERATIONS + index * 4);
        edu26_u64 acknowledged_request =
            edu26_read64(p + EDU26_META_ACK_REQUEST_IDS + index * 8);
        if (next_generation == 0 ||
            ((acknowledged_generation == 0) !=
             (acknowledged_request == 0)) ||
            (index < 4 &&
             (acknowledged_generation != 0 ||
              acknowledged_request != 0)) ||
            (acknowledged_generation != 0 &&
             acknowledged_generation >= next_generation)) return 0;
    }
    for (index = EDU26_META_RESERVED; index < EDU26_SECTOR - 4;
         index = index + 1) {
        if (p[index] != 0) return 0;
    }
    return edu26_fnv(p, EDU26_SECTOR - 4) ==
           edu26_read32(p + EDU26_SECTOR - 4);
}

edu26_u64 edu26_queue_entry_generation_action(
    const edu26_u8 *entry, const edu26_u8 *metadata, edu26_u64 slot) {
    edu26_u32 generation;
    edu26_u32 next_generation;
    edu26_u32 acknowledged_generation;
    edu26_u64 acknowledged_request;
    edu26_u32 state;
    if (entry == (const edu26_u8 *)0 ||
        metadata == (const edu26_u8 *)0 ||
        slot >= EDU26_ENTRY_COUNT) return EDU26_ERR_FORMAT;
    generation = edu26_read32(entry + 12);
    next_generation =
        edu26_read32(metadata + EDU26_META_NEXT_GENERATIONS + slot * 4);
    acknowledged_generation =
        edu26_read32(metadata + EDU26_META_ACK_GENERATIONS + slot * 4);
    acknowledged_request =
        edu26_read64(metadata + EDU26_META_ACK_REQUEST_IDS + slot * 8);
    if (generation == 0 || next_generation == 0 ||
        generation >= next_generation ||
        ((acknowledged_generation == 0) !=
         (acknowledged_request == 0))) return EDU26_ERR_FORMAT;
    if (acknowledged_generation == generation &&
        acknowledged_request == edu26_read64(entry + 16)) {
        state = edu26_read32(entry + 24);
        if (slot < 4 || state < EDU26_STATE_COMPLETE ||
            state > EDU26_STATE_CANCELLED) return EDU26_ERR_FORMAT;
        return EDU26_ACK_PREPARED;
    }
    return EDU26_OK;
}

int edu26_generation_reservable(edu26_u32 next_generation) {
    return next_generation != 0 && next_generation != 0xffffffffU;
}

int edu26_ack_identity_valid(
    edu26_u32 slot,
    edu26_u32 state,
    edu26_u32 entry_generation,
    edu26_u64 entry_request,
    edu26_u32 acknowledged_generation,
    edu26_u64 acknowledged_request) {
    if (slot < 4 || slot >= EDU26_ENTRY_COUNT ||
        state < EDU26_STATE_COMPLETE || state > EDU26_STATE_CANCELLED ||
        entry_generation == 0 || entry_request == 0) return 0;
    return acknowledged_generation == entry_generation &&
           acknowledged_request == entry_request;
}
