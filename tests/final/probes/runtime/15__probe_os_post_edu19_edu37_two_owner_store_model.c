/*
 * Compiler-side contract mirror derived from immutable os-dev tag
 * edu-37-bounded-two-owner-checkpoint-store, commit
 * 30e34df8ee4ddbc6ba8917c138dbd3403572a288.
 *
 * Authoritative implementation: queue64.asm
 * SHA-256: 5a38a3b75a7ac8ed54821c606fb3a44fce3462b52b3ddd302a474512194938bb.
 * queue_kernel.c is unchanged from EDU-36 at SHA-256
 * a9221eab3ce57bdd4516429b82fb15690d7be37350489bdbc1841cd2cfac9b1d.
 *
 * This bounded C mirror probes compiler behavior for the frozen selection
 * contract. It does not replace or claim source identity with the assembly.
 */
typedef unsigned char edu37_u8;
typedef unsigned int edu37_u32;
typedef unsigned long long edu37_u64;

extern edu37_u64 edu35_checkpoint_snapshot_valid(const edu37_u8 *bytes);
extern int edu32_workload_valid(const edu37_u8 *bytes, edu37_u64 length);

enum {
    EDU37_SECTOR_BYTES = 512,
    EDU37_LANE_COUNT = 2,
    EDU37_ENTRY_COUNT = 8,
    EDU37_STATE_RUNNING = 2,
    EDU37_STATE_FAILED = 4,
    EDU37_REASON_INTERRUPTED = 6,
    EDU37_CONSUMED_WORK_OFFSET = 496
};

static edu37_u32 edu37_read16(const edu37_u8 *bytes) {
    return (edu37_u32)bytes[0] | ((edu37_u32)bytes[1] << 8);
}

static edu37_u32 edu37_read32(const edu37_u8 *bytes) {
    return (edu37_u32)bytes[0] | ((edu37_u32)bytes[1] << 8) |
           ((edu37_u32)bytes[2] << 16) | ((edu37_u32)bytes[3] << 24);
}

static edu37_u64 edu37_read64(const edu37_u8 *bytes) {
    return (edu37_u64)edu37_read32(bytes) |
           ((edu37_u64)edu37_read32(bytes + 4) << 32);
}

static int edu37_all_zero(const edu37_u8 *bytes) {
    edu37_u32 index;
    for (index = 0; index < EDU37_SECTOR_BYTES; index = index + 1) {
        if (bytes[index] != 0) return 0;
    }
    return 1;
}

/*
 * Mirror queue64_validate_checkpoint_storage: each lane is either completely
 * zero or passes both nested validators, and two records may not name one
 * exact slot/generation/request owner.
 */
int edu37_checkpoint_storage_valid(const edu37_u8 *lanes) {
    edu37_u32 lane;
    const edu37_u8 *first = lanes;
    const edu37_u8 *second = lanes + EDU37_SECTOR_BYTES;
    for (lane = 0; lane < EDU37_LANE_COUNT; lane = lane + 1) {
        const edu37_u8 *record = lanes + lane * EDU37_SECTOR_BYTES;
        if (edu37_read64(record) == 0) {
            if (!edu37_all_zero(record)) return 0;
        } else {
            if (edu35_checkpoint_snapshot_valid(record) == 0 ||
                !edu32_workload_valid(record + 80, 104)) return 0;
        }
    }
    if (edu37_read64(first) != 0 && edu37_read64(second) != 0 &&
        edu37_read32(first + 16) == edu37_read32(second + 16) &&
        edu37_read32(first + 20) == edu37_read32(second + 20) &&
        edu37_read64(first + 24) == edu37_read64(second + 24)) {
        return 0;
    }
    return 1;
}

/*
 * Mirror queue64_select_checkpoint_lane. Return lane 0 or 1, or all-ones on
 * fail-closed capacity/identity failure. The caller has already run storage
 * validation, matching the immutable boot/publish ordering.
 */
edu37_u64 edu37_checkpoint_lane_select(
    const edu37_u8 *lanes, const edu37_u8 *entries,
    edu37_u32 target_slot, edu37_u32 target_generation,
    edu37_u64 target_request) {
    edu37_u64 first_empty = ~0ULL;
    edu37_u32 lane;
    for (lane = 0; lane < EDU37_LANE_COUNT; lane = lane + 1) {
        const edu37_u8 *record = lanes + lane * EDU37_SECTOR_BYTES;
        if (edu37_read64(record) == 0) {
            if (first_empty == ~0ULL) first_empty = lane;
        } else if (edu37_read32(record + 16) == target_slot &&
                   edu37_read32(record + 20) == target_generation &&
                   edu37_read64(record + 24) == target_request) {
            return lane;
        }
    }
    if (first_empty != ~0ULL) return first_empty;
    for (lane = 0; lane < EDU37_LANE_COUNT; lane = lane + 1) {
        const edu37_u8 *record = lanes + lane * EDU37_SECTOR_BYTES;
        edu37_u32 owner_slot = edu37_read32(record + 16);
        const edu37_u8 *entry;
        edu37_u32 state;
        if (owner_slot >= EDU37_ENTRY_COUNT) return ~0ULL;
        entry = entries + owner_slot * EDU37_SECTOR_BYTES;
        if (edu37_read64(entry) == 0 ||
            edu37_read32(entry + 12) != edu37_read32(record + 20) ||
            edu37_read64(entry + 16) != edu37_read64(record + 24)) {
            return lane;
        }
        state = edu37_read32(entry + 24);
        if (state == EDU37_STATE_RUNNING) continue;
        if (state == EDU37_STATE_FAILED &&
            edu37_read64(entry + 96) == EDU37_REASON_INTERRUPTED &&
            edu37_read16(entry + EDU37_CONSUMED_WORK_OFFSET) == 3) {
            continue;
        }
        return lane;
    }
    return ~0ULL;
}
