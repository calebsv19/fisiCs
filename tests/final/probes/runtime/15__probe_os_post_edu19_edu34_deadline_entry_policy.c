/*
 * Source: complete os-dev queue_kernel.c at immutable tag
 * edu-34-cooperative-relative-deadline-policy, commit
 * bf95c670b576b3b1f494463d5ec41c5af19cb5bc.
 * Origin SHA-256: d1b99e24c6e554207fb3e779e342a713ee996e1d74ebe01e3e98900d6e07591e.
 * The complete source below is unchanged from that snapshot.
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
    EDU30_FLAG_CANCEL_RUNNING = 2,
    EDU30_FLAGS_ALLOWED = 3,
    EDU15_OK = 0,
    EDU15_ERR_FORMAT = 1,
    EDU15_ERR_RESOURCE = 2,
    EDU15_CANCEL = 3,
    EDU30_META_VERSION = 9,
    EDU30_ENTRY_VERSION = 7,
    EDU27_TRACE_VERSION = 2,
    EDU22_TRACE_HEADER = 56,
    EDU22_TRACE_EVENTS = 104,
    EDU22_TRACE_EVENT_BYTES = 32,
    EDU22_TRACE_CAPACITY = 12,
    EDU22_TRACE_EPOCH = 488,
    EDU23_EFFECTIVE_WORKERS = 490,
    EDU30_REQUESTED_WORK = 492,
    EDU30_EFFECTIVE_WORK = 494,
    EDU30_CONSUMED_WORK = 496,
    EDU32_WORKLOAD_GENERATION = 498,
    EDU32_WORKLOAD_LENGTH = 502,
    EDU32_WORKLOAD_CHECKSUM = 504,
    EDU22_TRACE_FLAGS_ALLOWED = 15,
    EDU22_EVENT_RESOURCE_GRANTED = 6,
    EDU22_EVENT_RUNNING_PUBLISHED = 7,
    EDU22_EVENT_TERMINAL_COMPLETE = 11,
    EDU22_EVENT_TERMINAL_FAILED = 12,
    EDU22_EVENT_TERMINAL_CANCELLED = 13,
    EDU22_EVENT_RECOVERED_INTERRUPTED = 14,
    EDU27_EVENT_PHASE_SETUP_COMPLETE = 15,
    EDU27_EVENT_PHASE_COMPUTE_COMPLETE = 16,
    EDU27_EVENT_PHASE_BARRIER_COMPLETE = 17,
    EDU27_EVENT_PHASE_REDUCE_COMPLETE = 18,
    EDU30_REASON_CANCELLED = 3,
    EDU30_REASON_BUDGET = 8,
    EDU34_REASON_TIMEOUT = 9,
    EDU26_META_NEXT_GENERATIONS = 44,
    EDU26_META_ACK_GENERATIONS = 76,
    EDU26_META_ACK_REQUEST_IDS = 108,
    EDU26_META_RESERVED = 172,
    EDU26_ACK_PREPARED = 4
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
        read32(p + 8) != EDU30_META_VERSION) return EDU15_ERR_FORMAT;
    if (read32(p + 12) != entry_count || read32(p + 16) != entry_lba) return EDU15_ERR_FORMAT;
    if (read32(p + 20) != 0 ||
        read32(p + 24) != EDU30_ENTRY_VERSION ||
        read32(p + 28) != EDU15_SECTOR ||
        read32(p + 32) != EDU27_TRACE_VERSION ||
        read32(p + 36) != EDU22_TRACE_EVENT_BYTES ||
        read32(p + 40) != EDU22_TRACE_CAPACITY) return EDU15_ERR_FORMAT;
    {
        edu15_u64 index;
        for (index = 0; index < entry_count; index = index + 1) {
            edu15_u32 next_generation =
                read32(p + EDU26_META_NEXT_GENERATIONS + index * 4);
            edu15_u32 acknowledged_generation =
                read32(p + EDU26_META_ACK_GENERATIONS + index * 4);
            edu15_u64 acknowledged_request =
                read64(p + EDU26_META_ACK_REQUEST_IDS + index * 8);
            if (next_generation == 0) {
                return EDU15_ERR_FORMAT;
            }
            if ((acknowledged_generation == 0) != (acknowledged_request == 0)) {
                return EDU15_ERR_FORMAT;
            }
            if (index < 4 &&
                (acknowledged_generation != 0 || acknowledged_request != 0)) {
                return EDU15_ERR_FORMAT;
            }
            if (acknowledged_generation >= next_generation &&
                acknowledged_generation != 0) return EDU15_ERR_FORMAT;
        }
        for (index = EDU26_META_RESERVED; index < EDU15_SECTOR - 4;
             index = index + 1) {
            if (p[index] != 0) return EDU15_ERR_FORMAT;
        }
    }
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
    edu15_u32 seen;
    edu15_u32 phase_stage;
    edu15_u64 grant_value;
    edu15_u64 setup_value;
    edu15_u64 compute_value;
    edu15_u64 barrier_value;
    edu15_u64 reduce_value;
    edu15_u32 requested_workers;
    edu15_u32 effective_workers;
    edu15_u32 flags;
    edu15_u32 requested_work;
    edu15_u32 effective_work;
    edu15_u32 consumed_work;
    edu15_u32 last_reason;
    edu15_u64 last_value;
    edu15_u64 cancel_value;
    edu15_u64 deadline_duration;
    previous_sequence = 0;
    previous_epoch = 0;
    previous_tick = 0;
    last_kind = 0;
    seen = 0;
    phase_stage = 0;
    grant_value = 0;
    setup_value = 0;
    compute_value = 0;
    barrier_value = 0;
    reduce_value = 0;
    last_reason = 0;
    last_value = 0;
    cancel_value = 0;
    deadline_duration = 0;
    if (read64(p) != 0x00004A3531554445ULL ||
        read32(p + 8) != EDU30_ENTRY_VERSION ||
        read32(p + 12) == 0) return EDU15_ERR_FORMAT;
    if (fnv(p, EDU15_SECTOR - 4) != read32(p + 508)) return EDU15_ERR_FORMAT;
    state = read32(p + 24);
    if (state < EDU15_STATE_PENDING || state > EDU15_STATE_CANCELLED) {
        return EDU15_ERR_FORMAT;
    }
    flags = read32(p + 28);
    if ((flags & ~EDU30_FLAGS_ALLOWED) != 0) {
        return EDU15_ERR_FORMAT;
    }
    if (read64(p + 40) == 0 ||
        read64(p + 48) != 0x1E3C373BAF48FAF7ULL) return EDU15_ERR_FORMAT;
    if (read16(p + EDU22_TRACE_HEADER) != EDU27_TRACE_VERSION ||
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
            kind > EDU27_EVENT_PHASE_REDUCE_COMPLETE || kind == 8 || kind == 9 ||
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
        last_reason = read32(event + 20);
        last_value = read64(event + 24);
        if (kind == 2) {
            if ((seen & (1U << 2)) != 0 ||
                (read32(event + 16) != EDU15_STATE_PENDING &&
                 read32(event + 16) != EDU15_STATE_RUNNING)) {
                return EDU15_ERR_FORMAT;
            }
            cancel_value = last_value;
        }
        if (kind == 10 && read32(event + 16) != EDU15_STATE_RUNNING) {
            return EDU15_ERR_FORMAT;
        }
        seen = seen | (1U << kind);
        if (kind == EDU22_EVENT_RESOURCE_GRANTED) {
            grant_value = read64(event + 24);
        }
        if (kind == EDU27_EVENT_PHASE_SETUP_COMPLETE) {
            if (phase_stage != 0 || read32(event + 16) != EDU15_STATE_RUNNING ||
                read32(event + 20) != 1) return EDU15_ERR_FORMAT;
            phase_stage = 1;
            setup_value = read64(event + 24);
        }
        if (kind == EDU27_EVENT_PHASE_COMPUTE_COMPLETE) {
            if (phase_stage != 1 || read32(event + 16) != EDU15_STATE_RUNNING ||
                read32(event + 20) != 2) return EDU15_ERR_FORMAT;
            phase_stage = 2;
            compute_value = read64(event + 24);
        }
        if (kind == EDU27_EVENT_PHASE_BARRIER_COMPLETE) {
            if (phase_stage != 2 || read32(event + 16) != EDU15_STATE_RUNNING ||
                read32(event + 20) != 3) return EDU15_ERR_FORMAT;
            phase_stage = 3;
            barrier_value = read64(event + 24);
        }
        if (kind == EDU27_EVENT_PHASE_REDUCE_COMPLETE) {
            if (phase_stage != 3 || read32(event + 16) != EDU15_STATE_RUNNING ||
                read32(event + 20) != 4) return EDU15_ERR_FORMAT;
            phase_stage = 4;
            reduce_value = read64(event + 24);
        }
    }
    for (index = count * EDU22_TRACE_EVENT_BYTES;
         index < EDU22_TRACE_CAPACITY * EDU22_TRACE_EVENT_BYTES;
         index = index + 1) {
        if (p[EDU22_TRACE_EVENTS + index] != 0) return EDU15_ERR_FORMAT;
    }
    if (read16(p + EDU22_TRACE_EPOCH) != previous_epoch && count != 0) {
        return EDU15_ERR_FORMAT;
    }
    if (read32(p + EDU32_WORKLOAD_GENERATION) == 0 ||
        read16(p + EDU32_WORKLOAD_LENGTH) != 104 ||
        read32(p + EDU32_WORKLOAD_CHECKSUM) == 0) return EDU15_ERR_FORMAT;
    requested_workers = read32(p + 32);
    effective_workers = read16(p + EDU23_EFFECTIVE_WORKERS);
    requested_work = read16(p + EDU30_REQUESTED_WORK);
    effective_work = read16(p + EDU30_EFFECTIVE_WORK);
    consumed_work = read16(p + EDU30_CONSUMED_WORK);
    deadline_duration =
        (state == EDU15_STATE_PENDING || state == EDU15_STATE_RUNNING) ?
        read64(p + 96) : read64(p + 72);
    if (deadline_duration > 60000000000ULL) return EDU15_ERR_FORMAT;
    if (requested_work < 1 || requested_work > 4) return EDU15_ERR_FORMAT;
    if (state == EDU15_STATE_RUNNING && count != 0) {
        if (last_kind != EDU22_EVENT_RUNNING_PUBLISHED &&
            last_kind != 2 &&
            last_kind != EDU27_EVENT_PHASE_SETUP_COMPLETE &&
            last_kind != EDU27_EVENT_PHASE_COMPUTE_COMPLETE &&
            last_kind != EDU27_EVENT_PHASE_BARRIER_COMPLETE &&
            last_kind != EDU27_EVENT_PHASE_REDUCE_COMPLETE) {
            return EDU15_ERR_FORMAT;
        }
        if ((last_kind == EDU22_EVENT_RUNNING_PUBLISHED && phase_stage != 0) ||
            (last_kind == EDU27_EVENT_PHASE_SETUP_COMPLETE && phase_stage != 1) ||
            (last_kind == EDU27_EVENT_PHASE_COMPUTE_COMPLETE && phase_stage != 2) ||
            (last_kind == EDU27_EVENT_PHASE_BARRIER_COMPLETE && phase_stage != 3) ||
            (last_kind == EDU27_EVENT_PHASE_REDUCE_COMPLETE && phase_stage != 4) ||
            (last_kind == 2 && (flags & EDU30_FLAG_CANCEL_RUNNING) == 0)) {
            return EDU15_ERR_FORMAT;
        }
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
         effective_workers != 0 ||
         effective_work != 0 || consumed_work != 0 ||
         (flags & EDU30_FLAG_CANCEL_RUNNING) != 0)) return EDU15_ERR_FORMAT;
    if (state == EDU15_STATE_PENDING) {
        if ((flags & EDU15_FLAG_CANCEL_PENDING) != 0) {
            if (last_kind != 2 || cancel_value != requested_work) {
                return EDU15_ERR_FORMAT;
            }
        } else if ((seen & (1U << 2)) != 0) {
            return EDU15_ERR_FORMAT;
        }
    }
    if (state == EDU15_STATE_RUNNING &&
        (((phase_stage < 4) && read64(p + 64) != 0) ||
         ((phase_stage == 4) && read64(p + 64) != read64(p + 40)) ||
         read64(p + 72) == 0 ||
         read64(p + 80) == 0 || read64(p + 88) == 0)) return EDU15_ERR_FORMAT;
    if (state == EDU15_STATE_COMPLETE &&
        (read64(p + 64) != read64(p + 40) ||
         read64(p + 80) != 0 || read64(p + 88) != 0 ||
         read64(p + 96) != 0)) {
        return EDU15_ERR_FORMAT;
    }
    if ((state == EDU15_STATE_FAILED || state == EDU15_STATE_CANCELLED) &&
        (read64(p + 80) != 0 ||
         read64(p + 88) != 0 || read64(p + 96) == 0)) {
        return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_COMPLETE &&
        (requested_work != 4 || effective_work != 4 || consumed_work != 4 ||
         flags != 0 || last_reason != 0)) return EDU15_ERR_FORMAT;
    if (state == EDU15_STATE_RUNNING || state == EDU15_STATE_COMPLETE ||
        (state == EDU15_STATE_FAILED &&
         (seen & (1U << EDU22_EVENT_RUNNING_PUBLISHED)) != 0) ||
        (state == EDU15_STATE_CANCELLED &&
         (seen & (1U << EDU22_EVENT_RUNNING_PUBLISHED)) != 0)) {
        if (requested_workers < 1 || requested_workers > 2 ||
            effective_workers != requested_workers ||
            grant_value != ((edu15_u64)requested_workers |
                            ((edu15_u64)effective_workers << 32))) {
            return EDU15_ERR_FORMAT;
        }
    } else if (effective_workers != 0) {
        return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_RUNNING ||
        (state == EDU15_STATE_FAILED &&
         (seen & (1U << EDU22_EVENT_RUNNING_PUBLISHED)) != 0) ||
        (state == EDU15_STATE_CANCELLED &&
         (seen & (1U << EDU22_EVENT_RUNNING_PUBLISHED)) != 0) ||
        state == EDU15_STATE_COMPLETE) {
        if (effective_work != requested_work ||
            consumed_work != phase_stage) return EDU15_ERR_FORMAT;
    } else if (effective_work != 0 || consumed_work != 0) {
        return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_RUNNING) {
        if ((flags & EDU15_FLAG_CANCEL_PENDING) != 0) return EDU15_ERR_FORMAT;
        if ((flags & EDU30_FLAG_CANCEL_RUNNING) != 0) {
            edu15_u64 expected_budget =
                (edu15_u64)requested_work |
                ((edu15_u64)effective_work << 16) |
                ((edu15_u64)consumed_work << 32);
            if (last_kind != 2 || cancel_value != expected_budget) {
                return EDU15_ERR_FORMAT;
            }
        }
    }
    if (state == EDU15_STATE_CANCELLED) {
        edu15_u64 expected_budget =
            (edu15_u64)requested_work |
            ((edu15_u64)effective_work << 16) |
            ((edu15_u64)consumed_work << 32);
        if (last_reason != EDU30_REASON_CANCELLED ||
            last_value != expected_budget) return EDU15_ERR_FORMAT;
        if ((seen & (1U << EDU22_EVENT_RUNNING_PUBLISHED)) != 0) {
            if (flags != EDU30_FLAG_CANCEL_RUNNING ||
                effective_workers != requested_workers ||
                (seen & (1U << 2)) == 0 ||
                cancel_value != expected_budget ||
                ((phase_stage < 4 && read64(p + 64) != 0) ||
                 (phase_stage == 4 &&
                  read64(p + 64) != read64(p + 40)))) {
                return EDU15_ERR_FORMAT;
            }
        } else if (flags != EDU15_FLAG_CANCEL_PENDING ||
                   effective_workers != 0 ||
                   effective_work != 0 || consumed_work != 0) {
            return EDU15_ERR_FORMAT;
        }
    } else if (state != EDU15_STATE_RUNNING &&
               state != EDU15_STATE_PENDING && flags != 0) {
        return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_FAILED &&
        last_reason != read32(p + 96)) return EDU15_ERR_FORMAT;
    if (state == EDU15_STATE_FAILED &&
        read32(p + 96) == EDU30_REASON_BUDGET) {
        edu15_u64 expected_budget =
            (edu15_u64)requested_work |
            ((edu15_u64)effective_work << 16) |
            ((edu15_u64)consumed_work << 32);
        if ((seen & (1U << EDU22_EVENT_RUNNING_PUBLISHED)) == 0 ||
            phase_stage >= 4 || consumed_work != effective_work ||
            last_value != expected_budget) return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_FAILED &&
        read32(p + 96) == EDU34_REASON_TIMEOUT) {
        if (deadline_duration == 0 ||
            (seen & (1U << EDU22_EVENT_RUNNING_PUBLISHED)) == 0 ||
            last_value != read64(p + 72)) return EDU15_ERR_FORMAT;
    }
    if (state == EDU15_STATE_COMPLETE) {
        if ((seen & (1U << EDU22_EVENT_RESOURCE_GRANTED)) == 0 ||
            phase_stage != 4 ||
            (seen & (1U << EDU27_EVENT_PHASE_SETUP_COMPLETE)) == 0 ||
            (seen & (1U << EDU27_EVENT_PHASE_COMPUTE_COMPLETE)) == 0 ||
            (seen & (1U << EDU27_EVENT_PHASE_BARRIER_COMPLETE)) == 0 ||
            (seen & (1U << EDU27_EVENT_PHASE_REDUCE_COMPLETE)) == 0) {
            return EDU15_ERR_FORMAT;
        }
    }
    if (phase_stage != 0) {
        edu15_u64 expected_setup =
            (edu15_u64)effective_workers | ((edu15_u64)3 << 32);
        edu15_u64 expected_compute =
            ((edu15_u64)3 << 48) |
            (effective_workers == 1 ? (edu15_u64)6 :
             ((edu15_u64)3 | ((edu15_u64)1 << 16) |
              ((edu15_u64)3 << 32)));
        edu15_u64 expected_barrier =
            (edu15_u64)effective_workers |
            ((edu15_u64)effective_workers << 16) |
            ((edu15_u64)(effective_workers == 2 ? 1 : 0) << 32) |
            ((edu15_u64)3 << 48);
        if (setup_value != expected_setup) return EDU15_ERR_FORMAT;
        if (phase_stage >= 2 && compute_value != expected_compute) {
            return EDU15_ERR_FORMAT;
        }
        if (phase_stage >= 3 && barrier_value != expected_barrier) {
            return EDU15_ERR_FORMAT;
        }
        if (phase_stage >= 4 &&
            reduce_value != read64(p + 40)) {
            return EDU15_ERR_FORMAT;
        }
    }
    return EDU15_OK;
}

/*
 * Bind one validated live entry to the durable per-slot generation ledger.
 * EDU26_ACK_PREPARED means ACK metadata reached disk before its entry clear;
 * assembly may finish only that already-authorized reclaim transaction.
 */
edu15_u64 edu26_queue_entry_generation_action(const edu15_u8* entry,
                                               const edu15_u8* metadata,
                                               edu15_u64 slot) {
    edu15_u32 generation;
    edu15_u32 next_generation;
    edu15_u32 acknowledged_generation;
    edu15_u64 acknowledged_request;
    edu15_u32 state;
    if (slot >= 8) return EDU15_ERR_FORMAT;
    generation = read32(entry + 12);
    next_generation = read32(metadata + EDU26_META_NEXT_GENERATIONS + slot * 4);
    acknowledged_generation =
        read32(metadata + EDU26_META_ACK_GENERATIONS + slot * 4);
    acknowledged_request =
        read64(metadata + EDU26_META_ACK_REQUEST_IDS + slot * 8);
    if (generation == 0 || next_generation == 0 ||
        generation >= next_generation) return EDU15_ERR_FORMAT;
    if ((acknowledged_generation == 0) != (acknowledged_request == 0)) {
        return EDU15_ERR_FORMAT;
    }
    if (acknowledged_generation == generation &&
        acknowledged_request == read64(entry + 16)) {
        state = read32(entry + 24);
        if (slot < 4 || state < EDU15_STATE_COMPLETE ||
            state > EDU15_STATE_CANCELLED) return EDU15_ERR_FORMAT;
        return EDU26_ACK_PREPARED;
    }
    return EDU15_OK;
}

/* Return admission/cancellation only; assembly owns leases, state writes, and compute. */
edu15_u64 edu15_queue_entry_action(const edu15_u8* p, edu15_u64 cpu_count,
                                   edu15_u64 free_pages) {
    if (read32(p + 24) != EDU15_STATE_PENDING) return EDU15_ERR_FORMAT;
    if ((read32(p + 28) & EDU15_FLAG_CANCEL_PENDING) != 0) return EDU15_CANCEL;
    if ((read32(p + 32) != 1 && read32(p + 32) != 2) ||
        read32(p + 36) != 2 || cpu_count < 2 || free_pages < 2) {
        return EDU15_ERR_RESOURCE;
    }
    return EDU15_OK;
}

/* Hardware-blind validation of one fixed EDU-33 typed result sector. */
edu15_u64 edu33_result_payload_valid(const edu15_u8* p) {
    edu15_u64 index;
    if (read64(p) != 0x0031523333554445ULL ||
        read16(p + 8) != 1 || read16(p + 10) != 1 ||
        read32(p + 12) != 80 || read32(p + 16) >= 8 ||
        read32(p + 20) == 0 || read64(p + 24) == 0 ||
        read32(p + 32) == 0 || read16(p + 36) != 104 ||
        read16(p + 38) != 0 || read32(p + 40) == 0 ||
        read32(p + 44) != 0 || read32(p + 76) != 0) {
        return 0;
    }
    if (fnv(p, 72) != read32(p + 72)) return 0;
    for (index = 80; index < 508; index = index + 1) {
        if (p[index] != 0) return 0;
    }
    return fnv(p, 508) == read32(p + 508);
}
