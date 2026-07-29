/*
 * Compiler-side contract mirror derived from immutable os-dev tag
 * edu-39-bounded-smp-phase-context-ownership, commit
 * 6dd5cd27fe31221934d751220d5b36fa3f714b50.
 *
 * Authoritative implementation: smp64.asm
 * SHA-256: f0137844e6c666798e5af1b16cdc002d8d4a7f80370492f5bd4adee53c4fa1d9.
 * This C mirror probes the frozen 224-byte phase-owner shape and legal
 * switch/load boundaries only. It is not OS source, does not restore the
 * singleton AP mailbox, and does not raise the one-active-runner limit.
 */
typedef unsigned char edu39_u8;
typedef unsigned int edu39_u32;
typedef unsigned long long edu39_u64;

extern int edu32_workload_valid(const edu39_u8 *bytes, edu39_u64 length);

enum {
    EDU39_OWNER_COUNT = 2,
    EDU39_OWNER_BYTES = 224,
    EDU39_WORKLOAD_BYTES = 104,
    EDU39_QUEUE_SLOTS = 8,
    EDU39_PHASE_COMPUTE = 2,
    EDU39_PHASE_BARRIER = 3,
    EDU39_PHASE_REDUCE = 4,
    EDU39_WORK_PHASE_BATCH = 2
};

static edu39_u32 edu39_read16(const edu39_u8 *bytes) {
    return (edu39_u32)bytes[0] | ((edu39_u32)bytes[1] << 8);
}

static edu39_u32 edu39_read32(const edu39_u8 *bytes) {
    return (edu39_u32)bytes[0] | ((edu39_u32)bytes[1] << 8) |
           ((edu39_u32)bytes[2] << 16) | ((edu39_u32)bytes[3] << 24);
}

static edu39_u64 edu39_read64(const edu39_u8 *bytes) {
    return (edu39_u64)edu39_read32(bytes) |
           ((edu39_u64)edu39_read32(bytes + 4) << 32);
}

static edu39_u32 edu39_fnv1a32(const edu39_u8 *bytes, edu39_u32 count) {
    edu39_u32 value = 0x811C9DC5U;
    edu39_u32 index;
    for (index = 0; index < count; index = index + 1) {
        value = (value ^ bytes[index]) * 0x01000193U;
    }
    return value;
}

static int edu39_equal104(const edu39_u8 *left, const edu39_u8 *right) {
    edu39_u32 index;
    for (index = 0; index < EDU39_WORKLOAD_BYTES; index = index + 1) {
        if (left[index] != right[index]) return 0;
    }
    return 1;
}

edu39_u64 edu39_phase_owner_for_context(edu39_u64 context_id) {
    if (context_id >= EDU39_OWNER_COUNT) return ~0ULL;
    return context_id;
}

/*
 * Invalidated owners retain terminal evidence, so valid=0 requires only the
 * immutable lane identity. A valid owner must carry an exact bind tuple and
 * complete semantically valid Workload-v1 bytes.
 */
int edu39_phase_owner_record_valid(
    const edu39_u8 *record, edu39_u32 expected_id) {
    edu39_u32 valid;
    edu39_u32 phase;
    edu39_u32 reduced_valid;
    if (record == (const edu39_u8 *)0 || expected_id >= EDU39_OWNER_COUNT) {
        return 0;
    }
    valid = record[0];
    if (valid > 1 || record[1] != expected_id) return 0;
    if (valid == 0) return 1;

    phase = record[2];
    reduced_valid = record[3];
    if (phase > EDU39_PHASE_REDUCE || reduced_valid > 1 ||
        edu39_read32(record + 4) >= EDU39_QUEUE_SLOTS ||
        edu39_read32(record + 8) == 0 ||
        edu39_read32(record + 12) == 0 ||
        edu39_read64(record + 16) == 0 ||
        edu39_read16(record + 24) != EDU39_WORKLOAD_BYTES ||
        edu39_read16(record + 26) < 1 ||
        edu39_read16(record + 26) > 2 ||
        edu39_read32(record + 28) !=
            edu39_fnv1a32(record + 32, EDU39_WORKLOAD_BYTES) ||
        !edu32_workload_valid(record + 32, EDU39_WORKLOAD_BYTES)) {
        return 0;
    }
    if (phase == EDU39_PHASE_REDUCE) return reduced_valid == 1;
    return reduced_valid == 0;
}

int edu39_phase_owner_pair_valid(const edu39_u8 *owners) {
    if (owners == (const edu39_u8 *)0) return 0;
    return edu39_phase_owner_record_valid(owners, 0) &&
           edu39_phase_owner_record_valid(owners + EDU39_OWNER_BYTES, 1);
}

/*
 * The assembly may save width-two COMPUTE, but cannot load it because EDU-39
 * has no AP-mailbox restoration authority.
 */
int edu39_phase_owner_loadable(
    const edu39_u8 *record, edu39_u32 expected_id) {
    if (!edu39_phase_owner_record_valid(record, expected_id) ||
        record[0] == 0) return 0;
    return record[2] != EDU39_PHASE_COMPUTE ||
           edu39_read16(record + 26) != 2;
}

int edu39_phase_owner_matches(
    const edu39_u8 *record,
    edu39_u32 context_id,
    edu39_u32 slot,
    edu39_u32 queue_generation,
    edu39_u64 request_id,
    edu39_u32 workload_generation,
    edu39_u32 workload_length,
    edu39_u32 workload_checksum,
    edu39_u32 width,
    const edu39_u8 *workload) {
    if (!edu39_phase_owner_record_valid(record, context_id) ||
        record[0] == 0 || workload == (const edu39_u8 *)0) return 0;
    return edu39_read32(record + 4) == slot &&
           edu39_read32(record + 8) == queue_generation &&
           edu39_read32(record + 12) == workload_generation &&
           edu39_read64(record + 16) == request_id &&
           edu39_read16(record + 24) == workload_length &&
           edu39_read16(record + 26) == width &&
           edu39_read32(record + 28) == workload_checksum &&
           edu39_equal104(record + 32, workload);
}

/*
 * work_done is intentionally not consulted: the frozen switch guard rejects
 * a different width-two COMPUTE owner both before and after completion until
 * the selected owner crosses the joined barrier.
 */
int edu39_phase_switch_allowed(
    edu39_u64 current_context,
    edu39_u32 prospective_context,
    edu39_u32 phase,
    edu39_u32 width,
    edu39_u64 work_done) {
    (void)work_done;
    if (prospective_context >= EDU39_OWNER_COUNT) return 0;
    if (current_context == ~0ULL) return 1;
    if (current_context >= EDU39_OWNER_COUNT) return 0;
    if (current_context == prospective_context) return 1;
    return phase != EDU39_PHASE_COMPUTE || width != 2;
}

int edu39_phase_inflight(
    edu39_u64 current_context,
    edu39_u32 phase,
    edu39_u32 width,
    edu39_u32 work_mode,
    edu39_u64 work_done) {
    return current_context < EDU39_OWNER_COUNT &&
           phase == EDU39_PHASE_COMPUTE &&
           width == 2 &&
           work_mode == EDU39_WORK_PHASE_BATCH &&
           work_done < 1;
}

/*
 * Classify exact publication evidence: 1=width one, 2=width two,
 * 3=resumed joined barrier, 0=not an exact publication shape.
 */
int edu39_phase_publication_path_class(const edu39_u8 *record) {
    edu39_u64 bsp;
    edu39_u64 dispatches;
    edu39_u64 completions;
    edu39_u64 ap_calls;
    edu39_u32 width;
    if (record == (const edu39_u8 *)0 ||
        record[0] != 1 ||
        record[2] < EDU39_PHASE_BARRIER ||
        record[2] > EDU39_PHASE_REDUCE) return 0;
    width = edu39_read16(record + 26);
    bsp = edu39_read64(record + 192);
    dispatches = edu39_read64(record + 200);
    completions = edu39_read64(record + 208);
    ap_calls = edu39_read64(record + 216);
    if (bsp == 0 && dispatches == 0 && completions == 0 && ap_calls == 0) {
        return 3;
    }
    if (width == 1 && bsp == 6 && dispatches == 0 &&
        completions == 0 && ap_calls == 0) return 1;
    if (width == 2 && bsp == 3 && dispatches == 1 &&
        completions == 1 && ap_calls == 3) return 2;
    return 0;
}
