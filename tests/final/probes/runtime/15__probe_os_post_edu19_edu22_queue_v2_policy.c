/*
 * Exact queue_kernel.c from immutable os-dev tag
 * edu-22-bounded-run-observation, commit
 * 37900baa5e80f219bd18964b2c606373e908220a.
 */
typedef unsigned char edu15_u8;
typedef unsigned int edu15_u32;
typedef unsigned long long edu15_u64;

enum {
    EDU15_SECTOR = 512,
    EDU15_STATE_EMPTY = 0,
    EDU15_STATE_PENDING = 1,
    EDU15_STATE_RUNNING = 2,
    EDU15_STATE_COMPLETE = 3,
    EDU15_STATE_FAILED = 4,
    EDU15_STATE_CANCELLED = 5,
    EDU15_FLAG_CANCEL_PENDING = 1,
    EDU15_OK = 0,
    EDU15_ERR_FORMAT = 1,
    EDU15_ERR_RESOURCE = 2,
    EDU15_CANCEL = 3,
    EDU22_META_VERSION = 3,
    EDU22_ENTRY_VERSION = 2,
    EDU22_TRACE_VERSION = 1,
    EDU22_TRACE_HEADER = 56,
    EDU22_TRACE_EVENTS = 104,
    EDU22_TRACE_EVENT_BYTES = 32,
    EDU22_TRACE_CAPACITY = 12,
    EDU22_TRACE_EPOCH = 488,
    EDU22_TRACE_FLAGS_ALLOWED = 15,
    EDU22_EVENT_TERMINAL_COMPLETE = 11,
    EDU22_EVENT_TERMINAL_FAILED = 12,
    EDU22_EVENT_TERMINAL_CANCELLED = 13,
    EDU22_EVENT_RECOVERED_INTERRUPTED = 14
};

static edu15_u32 read16(const edu15_u8* p) {
    return (edu15_u32)p[0] | ((edu15_u32)p[1] << 8);
}

static edu15_u32 read32(const edu15_u8* p) {
    return (edu15_u32)p[0] | ((edu15_u32)p[1] << 8) |
           ((edu15_u32)p[2] << 16) | ((edu15_u32)p[3] << 24);
}

static edu15_u64 read64(const edu15_u8* p) {
    return (edu15_u64)read32(p) | ((edu15_u64)read32(p + 4) << 32);
}

static edu15_u32 fnv(const edu15_u8* p, edu15_u64 count) {
    edu15_u32 value = 0x811C9DC5U;
    edu15_u64 index;
    for (index = 0; index < count; index = index + 1) value = (value ^ p[index]) * 0x01000193U;
    return value;
}

/* Hardware-blind validation of the fixed, build-authored EDU-15 queue header. */
edu15_u64 edu15_queue_meta_valid(const edu15_u8* p, edu15_u64 entry_lba,
                                  edu15_u64 entry_count) {
    if (read64(p) != 0x0000513531554445ULL ||
        read32(p + 8) != EDU22_META_VERSION) return EDU15_ERR_FORMAT;
    if (read32(p + 12) != entry_count || read32(p + 16) != entry_lba) return EDU15_ERR_FORMAT;
    if (read32(p + 20) != 0 ||
        read32(p + 24) != EDU22_ENTRY_VERSION ||
        read32(p + 28) != EDU15_SECTOR ||
        read32(p + 32) != EDU22_TRACE_VERSION ||
        read32(p + 36) != EDU22_TRACE_EVENT_BYTES ||
        read32(p + 40) != EDU22_TRACE_CAPACITY) return EDU15_ERR_FORMAT;
    return fnv(p, EDU15_SECTOR - 4) == read32(p + 508) ? EDU15_OK : EDU15_ERR_FORMAT;
}

/* Validate the complete durable entry before any state, result, or trace use. */
edu15_u64 edu22_queue_entry_valid(const edu15_u8* p) {
    edu15_u64 index;
    edu15_u32 count;
    edu15_u32 state;
    edu15_u32 previous_sequence;
    edu15_u32 previous_epoch;
    edu15_u64 previous_tick;
    edu15_u32 last_kind;
    previous_sequence = 0;
    previous_epoch = 0;
    previous_tick = 0;
    last_kind = 0;
    if (read64(p) != 0x00004A3531554445ULL ||
        read32(p + 8) != EDU22_ENTRY_VERSION ||
        read32(p + 12) == 0 || read32(p + 12) > 8) return EDU15_ERR_FORMAT;
    if (fnv(p, EDU15_SECTOR - 4) != read32(p + 508)) return EDU15_ERR_FORMAT;
    state = read32(p + 24);
    if (state < EDU15_STATE_PENDING || state > EDU15_STATE_CANCELLED) {
        return EDU15_ERR_FORMAT;
    }
    if ((read32(p + 28) & ~EDU15_FLAG_CANCEL_PENDING) != 0) {
        return EDU15_ERR_FORMAT;
    }
    if (read64(p + 40) != 0x6EC4E5DB9E1056CFULL ||
        read64(p + 48) != 0x1E3C373BAF48FAF7ULL) return EDU15_ERR_FORMAT;
    if (read16(p + EDU22_TRACE_HEADER) != EDU22_TRACE_VERSION ||
        read16(p + EDU22_TRACE_HEADER + 2) != EDU22_TRACE_EVENT_BYTES ||
        p[EDU22_TRACE_HEADER + 5] != EDU22_TRACE_CAPACITY ||
        (read16(p + EDU22_TRACE_HEADER + 6) & ~EDU22_TRACE_FLAGS_ALLOWED) != 0) {
        return EDU15_ERR_FORMAT;
    }
    count = p[EDU22_TRACE_HEADER + 4];
    if (count > EDU22_TRACE_CAPACITY) return EDU15_ERR_FORMAT;
    for (index = 0; index < count; index = index + 1) {
        const edu15_u8* event = p + EDU22_TRACE_EVENTS +
                                index * EDU22_TRACE_EVENT_BYTES;
        edu15_u32 sequence = read16(event);
        edu15_u32 epoch = read16(event + 2);
        edu15_u32 kind = read16(event + 4);
        edu15_u64 tick = read64(event + 8);
        if (sequence <= previous_sequence || kind == 0 ||
            kind > EDU22_EVENT_RECOVERED_INTERRUPTED ||
            event[7] > 1 || read32(event + 16) > EDU15_STATE_CANCELLED) {
            return EDU15_ERR_FORMAT;
        }
        if (index != 0 && epoch < previous_epoch) return EDU15_ERR_FORMAT;
        if (index != 0 && epoch == previous_epoch && tick < previous_tick) {
            return EDU15_ERR_FORMAT;
        }
        previous_sequence = sequence;
        previous_epoch = epoch;
        previous_tick = tick;
        last_kind = kind;
    }
    for (index = count * EDU22_TRACE_EVENT_BYTES;
         index < EDU22_TRACE_CAPACITY * EDU22_TRACE_EVENT_BYTES;
         index = index + 1) {
        if (p[EDU22_TRACE_EVENTS + index] != 0) return EDU15_ERR_FORMAT;
    }
    if (read16(p + EDU22_TRACE_EPOCH) != previous_epoch && count != 0) {
        return EDU15_ERR_FORMAT;
    }
    for (index = EDU22_TRACE_EPOCH + 2; index < EDU15_SECTOR - 4;
         index = index + 1) {
        if (p[index] != 0) return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_RUNNING && count != 0 && last_kind != 7) {
        return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_COMPLETE &&
        (count == 0 || last_kind != EDU22_EVENT_TERMINAL_COMPLETE)) {
        return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_FAILED &&
        (count == 0 || last_kind != EDU22_EVENT_TERMINAL_FAILED)) {
        return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_CANCELLED &&
        (count == 0 || last_kind != EDU22_EVENT_TERMINAL_CANCELLED)) {
        return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_PENDING &&
        (read64(p + 64) != 0 || read64(p + 72) != 0 ||
         read64(p + 80) != 0 || read64(p + 88) != 0 ||
         read64(p + 96) != 0)) return EDU15_ERR_FORMAT;
    if (state == EDU15_STATE_RUNNING &&
        (read64(p + 64) != 0 || read64(p + 72) == 0 ||
         read64(p + 80) == 0 || read64(p + 88) == 0 ||
         read64(p + 96) != 0)) return EDU15_ERR_FORMAT;
    if (state == EDU15_STATE_COMPLETE &&
        (read64(p + 64) != 0x6EC4E5DB9E1056CFULL ||
         read64(p + 72) != 0 || read64(p + 80) != 0 ||
         read64(p + 88) != 0 || read64(p + 96) != 0)) {
        return EDU15_ERR_FORMAT;
    }
    if ((state == EDU15_STATE_FAILED || state == EDU15_STATE_CANCELLED) &&
        (read64(p + 72) != 0 || read64(p + 80) != 0 ||
         read64(p + 88) != 0 || read64(p + 96) == 0)) {
        return EDU15_ERR_FORMAT;
    }
    return EDU15_OK;
}

/* Return admission/cancellation only; assembly owns leases, state writes, and compute. */
edu15_u64 edu15_queue_entry_action(const edu15_u8* p, edu15_u64 cpu_count,
                                   edu15_u64 free_pages) {
    if (read32(p + 24) != EDU15_STATE_PENDING) return EDU15_ERR_FORMAT;
    if ((read32(p + 28) & EDU15_FLAG_CANCEL_PENDING) != 0) return EDU15_CANCEL;
    if (read32(p + 32) != 2 || read32(p + 36) != 2 || cpu_count < 2 || free_pages < 2) {
        return EDU15_ERR_RESOURCE;
    }
    return EDU15_OK;
}
