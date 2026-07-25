#include <stdio.h>
#include <stdint.h>

#include "15__probe_osp3_raw_elf_policy.h"

#ifndef OSP3_RAW_MODE
#define OSP3_RAW_MODE 0
#endif

#ifndef OSP3_RAW_SEED
#define OSP3_RAW_SEED 0x41c6ce57u
#endif

#ifndef OSP3_RAW_CASE_BUDGET
#define OSP3_RAW_CASE_BUDGET 256u
#endif

#define OSP3_IMAGE_CAPACITY 1024u
#define OSP3_BASE_IMAGE_SIZE 0x240u
#define OSP3_PH_OFFSET 64u
#define OSP3_PH_SIZE 56u

struct osp3_stats {
    uint32_t cases;
    uint32_t accepts;
    uint32_t rejects;
    uint32_t failures;
    uint32_t digest;
};

static void put_u16(uint8_t *bytes, uint16_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t *bytes, uint32_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void put_u64(uint8_t *bytes, uint64_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
    bytes[4] = (uint8_t)(value >> 32);
    bytes[5] = (uint8_t)(value >> 40);
    bytes[6] = (uint8_t)(value >> 48);
    bytes[7] = (uint8_t)(value >> 56);
}

static void clear_image(uint8_t *image) {
    uint32_t i;
    for (i = 0; i < OSP3_IMAGE_CAPACITY; ++i) {
        image[i] = 0;
    }
}

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint32_t count) {
    uint32_t i;
    for (i = 0; i < count; ++i) {
        dst[i] = src[i];
    }
}

static uint8_t *ph_at(uint8_t *image, uint32_t index) {
    return image + OSP3_PH_OFFSET + index * OSP3_PH_SIZE;
}

static void put_ph(
    uint8_t *ph,
    uint32_t type,
    uint32_t flags,
    uint64_t offset,
    uint64_t vaddr,
    uint64_t filesz,
    uint64_t memsz
) {
    put_u32(ph, type);
    put_u32(ph + 4, flags);
    put_u64(ph + 8, offset);
    put_u64(ph + 16, vaddr);
    put_u64(ph + 24, vaddr);
    put_u64(ph + 32, filesz);
    put_u64(ph + 40, memsz);
    put_u64(ph + 48, 0x1000u);
}

static void build_base(uint8_t *image) {
    clear_image(image);
    image[0] = 0x7f;
    image[1] = 'E';
    image[2] = 'L';
    image[3] = 'F';
    image[4] = 2;
    image[5] = 1;
    image[6] = 1;
    image[7] = 0;
    image[8] = 0;
    put_u16(image + 16, 2);
    put_u16(image + 18, 62);
    put_u32(image + 20, 1);
    put_u64(image + 24, 0x10000120u);
    put_u64(image + 32, OSP3_PH_OFFSET);
    put_u16(image + 52, 64);
    put_u16(image + 54, OSP3_PH_SIZE);
    put_u16(image + 56, 2);
    put_ph(
        ph_at(image, 0),
        1,
        5,
        0x100u,
        0x10000100u,
        0x80u,
        0xf00u
    );
    put_ph(
        ph_at(image, 1),
        1,
        6,
        0x200u,
        0x10002200u,
        0x40u,
        0xe00u
    );
}

static uint32_t next_u32(uint32_t *state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static uint32_t mix(uint32_t hash, uint64_t value) {
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    hash ^= low + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    hash ^= high + 0x85ebca6bu + (hash << 5) + (hash >> 3);
    return hash;
}

static int reject_state_is_clean(
    const struct osp3_raw_elf_state *state,
    uint32_t reason
) {
    uint32_t i;
    if (
        state->accepted != 0 ||
        state->reason != reason ||
        state->load_count != 0 ||
        state->executable_count != 0 ||
        state->entry_segment != UINT32_MAX ||
        state->min_vaddr != 0 ||
        state->max_vaddr != 0 ||
        state->file_bytes != 0 ||
        state->memory_bytes != 0
    ) {
        return 0;
    }
    for (i = 0; i < OSP3_RAW_MAX_LOADS; ++i) {
        if (state->load_start[i] != 0 || state->load_end[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static int accept_state_is_coherent(
    const struct osp3_raw_elf_state *state
) {
    return (
        state->accepted == 1 &&
        state->reason == OSP3_RAW_ACCEPT &&
        state->load_count >= 1 &&
        state->load_count <= OSP3_RAW_MAX_LOADS &&
        state->executable_count >= 1 &&
        state->entry_segment < state->load_count &&
        state->min_vaddr >= 0x10000000u &&
        state->max_vaddr <= 0x10700000u &&
        state->min_vaddr < state->max_vaddr
    );
}

static void record_case(
    struct osp3_stats *stats,
    uint32_t actual,
    uint32_t expected,
    const struct osp3_raw_elf_state *state
) {
    uint32_t coherent;
#ifdef OSP3_RAW_TRACE
    printf(
        "trace case=%u actual=%u expected=%u reason=%u loads=%u entry=%u\n",
        (unsigned)stats->cases,
        (unsigned)actual,
        (unsigned)expected,
        (unsigned)state->reason,
        (unsigned)state->load_count,
        (unsigned)state->entry_segment
    );
#endif
    ++stats->cases;
    if (actual == OSP3_RAW_ACCEPT) {
        ++stats->accepts;
        coherent = (uint32_t)accept_state_is_coherent(state);
    } else {
        ++stats->rejects;
        coherent = (uint32_t)reject_state_is_clean(state, actual);
    }
    if (actual != expected || coherent == 0) {
        ++stats->failures;
    }
    stats->digest = mix(stats->digest, actual);
    stats->digest = mix(stats->digest, expected);
    stats->digest = mix(stats->digest, coherent);
    stats->digest = mix(stats->digest, state->load_count);
    stats->digest = mix(stats->digest, state->entry_segment);
    stats->digest = mix(stats->digest, state->min_vaddr);
    stats->digest = mix(stats->digest, state->max_vaddr);
    stats->digest = mix(stats->digest, state->file_bytes);
    stats->digest = mix(stats->digest, state->memory_bytes);
}

static void record_observation(
    struct osp3_stats *stats,
    uint32_t actual,
    const struct osp3_raw_elf_state *state
) {
    uint32_t coherent;
    ++stats->cases;
    if (actual == OSP3_RAW_ACCEPT) {
        ++stats->accepts;
        coherent = (uint32_t)accept_state_is_coherent(state);
    } else {
        ++stats->rejects;
        coherent = (uint32_t)reject_state_is_clean(state, actual);
    }
    if (coherent == 0) {
        ++stats->failures;
    }
    stats->digest = mix(stats->digest, actual);
    stats->digest = mix(stats->digest, coherent);
    stats->digest = mix(stats->digest, state->load_count);
    stats->digest = mix(stats->digest, state->entry_segment);
    stats->digest = mix(stats->digest, state->min_vaddr);
    stats->digest = mix(stats->digest, state->max_vaddr);
}

static void run_valid_topology(struct osp3_stats *stats) {
    uint8_t image[OSP3_IMAGE_CAPACITY];
    uint8_t saved[OSP3_PH_SIZE];
    struct osp3_raw_elf_state state;
    uint32_t actual;

    build_base(image);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u16(image + 56, 1);
    actual = osp3_raw_elf_admit(image, 0x180u, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u16(image + 56, 3);
    put_ph(ph_at(image, 2), 4, 0, 0, 0, 0, 0);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u16(image + 56, 3);
    put_ph(
        ph_at(image, 2),
        1,
        4,
        0x300u,
        0x10004300u,
        0x20u,
        0xd00u
    );
    actual = osp3_raw_elf_admit(image, 0x320u, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u64(ph_at(image, 1) + 32, 0);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u64(ph_at(image, 1) + 16, 0x106ff200u);
    put_u64(ph_at(image, 1) + 24, 0x106ff200u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    copy_bytes(saved, ph_at(image, 0), OSP3_PH_SIZE);
    copy_bytes(ph_at(image, 0), ph_at(image, 1), OSP3_PH_SIZE);
    copy_bytes(ph_at(image, 1), saved, OSP3_PH_SIZE);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u16(image + 56, 4);
    put_ph(
        ph_at(image, 2),
        1,
        4,
        0x300u,
        0x10004300u,
        0x20u,
        0xd00u
    );
    put_ph(
        ph_at(image, 3),
        1,
        6,
        0x400u,
        0x10006400u,
        0x20u,
        0xc00u
    );
    actual = osp3_raw_elf_admit(image, 0x420u, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);
}

static void run_header_and_truncation(struct osp3_stats *stats) {
    uint8_t image[OSP3_IMAGE_CAPACITY];
    struct osp3_raw_elf_state state;
    uint32_t size;
    uint32_t actual;

    for (size = 0; size <= 600u; ++size) {
        uint32_t expected;
        build_base(image);
        actual = osp3_raw_elf_admit(image, size, &state);
        if (size < 64u) {
            expected = OSP3_RAW_REJECT_SHORT;
        } else if (size < 176u) {
            expected = OSP3_RAW_REJECT_PHDR_GEOMETRY;
        } else if (size < OSP3_BASE_IMAGE_SIZE) {
            expected = OSP3_RAW_REJECT_FILE_RANGE;
        } else {
            expected = OSP3_RAW_ACCEPT;
        }
        record_case(stats, actual, expected, &state);
    }

#define OSP3_IDENT_CASE(offset, value) \
    build_base(image); \
    image[(offset)] = (value); \
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state); \
    record_case(stats, actual, OSP3_RAW_REJECT_IDENT, &state)

    OSP3_IDENT_CASE(0, 0);
    OSP3_IDENT_CASE(1, 'X');
    OSP3_IDENT_CASE(4, 1);
    OSP3_IDENT_CASE(5, 2);
    OSP3_IDENT_CASE(6, 0);
    OSP3_IDENT_CASE(7, 3);
    OSP3_IDENT_CASE(8, 1);
#undef OSP3_IDENT_CASE

    build_base(image);
    put_u16(image + 16, 3);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_HEADER, &state);

    build_base(image);
    put_u16(image + 18, 3);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_HEADER, &state);

    build_base(image);
    put_u32(image + 20, 0);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_HEADER, &state);

    build_base(image);
    put_u16(image + 52, 63);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_HEADER, &state);

    build_base(image);
    put_u16(image + 54, 55);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_PHDR_GEOMETRY, &state);

    build_base(image);
    put_u16(image + 56, 0);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_PHDR_GEOMETRY, &state);

    build_base(image);
    put_u16(image + 56, 9);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_PHDR_GEOMETRY, &state);

    build_base(image);
    put_u64(image + 32, UINT64_MAX);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_PHDR_GEOMETRY, &state);
}

static void run_overlap(struct osp3_stats *stats) {
    uint8_t image[OSP3_IMAGE_CAPACITY];
    uint8_t saved[OSP3_PH_SIZE];
    struct osp3_raw_elf_state state;
    uint32_t actual;

    build_base(image);
    put_u64(ph_at(image, 0) + 40, 0x1100u);
    put_u64(ph_at(image, 1) + 16, 0x10001200u);
    put_u64(ph_at(image, 1) + 24, 0x10001200u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u64(ph_at(image, 1) + 8, 0x180u);
    put_u64(ph_at(image, 1) + 16, 0x10000180u);
    put_u64(ph_at(image, 1) + 24, 0x10000180u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_OVERLAP, &state);

    build_base(image);
    put_u64(ph_at(image, 1) + 8, 0x100u);
    put_u64(ph_at(image, 1) + 16, 0x10000100u);
    put_u64(ph_at(image, 1) + 24, 0x10000100u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_OVERLAP, &state);

    build_base(image);
    put_u64(ph_at(image, 1) + 8, 0x180u);
    put_u64(ph_at(image, 1) + 16, 0x10000180u);
    put_u64(ph_at(image, 1) + 24, 0x10000180u);
    put_u64(ph_at(image, 1) + 40, 0x200u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_OVERLAP, &state);

    build_base(image);
    copy_bytes(saved, ph_at(image, 0), OSP3_PH_SIZE);
    copy_bytes(ph_at(image, 0), ph_at(image, 1), OSP3_PH_SIZE);
    copy_bytes(ph_at(image, 1), saved, OSP3_PH_SIZE);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u16(image + 56, 3);
    put_ph(ph_at(image, 2), 4, 0, 0x100u, 0x10000000u, 0, 0);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);
}

static void run_entry_membership(struct osp3_stats *stats) {
    uint8_t image[OSP3_IMAGE_CAPACITY];
    struct osp3_raw_elf_state state;
    uint32_t actual;

    build_base(image);
    put_u64(image + 24, 0x10000100u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u64(image + 24, 0x1000017fu);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u64(image + 24, 0x10000180u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_ENTRY, &state);

    build_base(image);
    put_u64(image + 24, 0x10002200u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_ENTRY, &state);

    build_base(image);
    put_u64(image + 24, 0x10001800u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_ENTRY, &state);

    build_base(image);
    put_u32(ph_at(image, 0) + 4, 4);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_ENTRY, &state);

    build_base(image);
    put_u32(ph_at(image, 1) + 4, 5);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

    build_base(image);
    put_u32(ph_at(image, 0) + 4, 7);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_FLAGS, &state);

    build_base(image);
    put_u64(image + 24, UINT64_MAX);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_ENTRY, &state);
}

static void run_reset_sequences(struct osp3_stats *stats) {
    uint8_t image[OSP3_IMAGE_CAPACITY];
    struct osp3_raw_elf_state state;
    uint32_t cycle;
    uint32_t actual;

    for (cycle = 0; cycle < 16u; ++cycle) {
        build_base(image);
        actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
        record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

        image[0] = 0;
        actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
        record_case(stats, actual, OSP3_RAW_REJECT_IDENT, &state);

        build_base(image);
        put_u16(image + 56, 1);
        actual = osp3_raw_elf_admit(image, 0x180u, &state);
        record_case(stats, actual, OSP3_RAW_ACCEPT, &state);

        build_base(image);
        put_u64(ph_at(image, 1) + 8, 0x100u);
        put_u64(ph_at(image, 1) + 16, 0x10000100u);
        put_u64(ph_at(image, 1) + 24, 0x10000100u);
        actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
        record_case(stats, actual, OSP3_RAW_REJECT_OVERLAP, &state);
    }
}

static void run_overflow_geometry(struct osp3_stats *stats) {
    uint8_t image[OSP3_IMAGE_CAPACITY];
    struct osp3_raw_elf_state state;
    uint32_t actual;

    build_base(image);
    put_u64(image + 32, UINT64_MAX);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_PHDR_GEOMETRY, &state);

    build_base(image);
    put_u64(image + 32, OSP3_BASE_IMAGE_SIZE);
    put_u16(image + 56, 1);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_PHDR_GEOMETRY, &state);

    build_base(image);
    put_u64(ph_at(image, 0) + 8, UINT64_MAX);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_FILE_RANGE, &state);

    build_base(image);
    put_u64(ph_at(image, 0) + 32, UINT64_MAX);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_FILE_RANGE, &state);

    build_base(image);
    put_u64(ph_at(image, 0) + 32, 0x1001u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_FILE_RANGE, &state);

    build_base(image);
    put_u64(ph_at(image, 0) + 16, UINT64_MAX);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_MEMORY_RANGE, &state);

    build_base(image);
    put_u64(ph_at(image, 0) + 16, 0x10700000u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_MEMORY_RANGE, &state);

    build_base(image);
    put_u64(ph_at(image, 0) + 40, UINT64_MAX);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_MEMORY_RANGE, &state);

    build_base(image);
    put_u64(ph_at(image, 0) + 16, 0x106ff000u);
    put_u64(ph_at(image, 0) + 24, 0x106ff000u);
    put_u64(ph_at(image, 0) + 40, 0x1001u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_MEMORY_RANGE, &state);

    build_base(image);
    put_u64(ph_at(image, 0) + 8, 0x101u);
    actual = osp3_raw_elf_admit(image, OSP3_BASE_IMAGE_SIZE, &state);
    record_case(stats, actual, OSP3_RAW_REJECT_CONGRUENCE, &state);
}

static void run_mutations(struct osp3_stats *stats) {
    uint8_t image[OSP3_IMAGE_CAPACITY];
    struct osp3_raw_elf_state state;
    uint32_t seed = OSP3_RAW_SEED;
    uint32_t index;

    for (index = 0; index < OSP3_RAW_CASE_BUDGET; ++index) {
        uint32_t bits;
        uint32_t kind;
        uint64_t image_size = OSP3_BASE_IMAGE_SIZE;
        uint32_t actual;
        build_base(image);
        bits = next_u32(&seed);
        kind = bits & 15u;
        switch (kind) {
            case 0:
                image[bits % 9u] ^= (uint8_t)(1u << ((bits >> 8) & 7u));
                break;
            case 1:
                put_u16(image + 16, (uint16_t)(bits >> 8));
                break;
            case 2:
                put_u16(image + 18, (uint16_t)(bits >> 8));
                break;
            case 3:
                put_u64(image + 24, ((uint64_t)next_u32(&seed) << 32) | bits);
                break;
            case 4:
                put_u64(image + 32, ((uint64_t)next_u32(&seed) << 32) | bits);
                break;
            case 5:
                put_u16(image + 56, (uint16_t)(bits >> 8));
                break;
            case 6:
                put_u32(ph_at(image, (bits >> 8) & 1u) + 4, bits >> 16);
                break;
            case 7:
                put_u64(
                    ph_at(image, (bits >> 8) & 1u) + 8,
                    ((uint64_t)next_u32(&seed) << 32) | bits
                );
                break;
            case 8:
                put_u64(
                    ph_at(image, (bits >> 8) & 1u) + 16,
                    ((uint64_t)next_u32(&seed) << 32) | bits
                );
                break;
            case 9:
                put_u64(
                    ph_at(image, (bits >> 8) & 1u) + 32,
                    ((uint64_t)next_u32(&seed) << 32) | bits
                );
                break;
            case 10:
                put_u64(
                    ph_at(image, (bits >> 8) & 1u) + 40,
                    ((uint64_t)next_u32(&seed) << 32) | bits
                );
                break;
            case 11:
                put_u32(ph_at(image, (bits >> 8) & 1u), bits >> 16);
                break;
            case 12:
                image_size = bits % (OSP3_BASE_IMAGE_SIZE + 1u);
                break;
            case 13:
                image[0x100u + (bits & 0x7fu)] ^= (uint8_t)(bits >> 24);
                break;
            case 14:
                put_u32(ph_at(image, (bits >> 8) & 1u), 4);
                break;
            default:
                image[64u + (bits % 112u)] ^= (uint8_t)(bits >> 24);
                break;
        }
        actual = osp3_raw_elf_admit(image, image_size, &state);
        record_observation(stats, actual, &state);
    }
}

int main(void) {
    struct osp3_stats stats;
    stats.cases = 0;
    stats.accepts = 0;
    stats.rejects = 0;
    stats.failures = 0;
    stats.digest = 0x811c9dc5u;

#if OSP3_RAW_MODE == 0
    run_valid_topology(&stats);
#elif OSP3_RAW_MODE == 1
    run_header_and_truncation(&stats);
#elif OSP3_RAW_MODE == 2
    run_overlap(&stats);
#elif OSP3_RAW_MODE == 3
    run_entry_membership(&stats);
#elif OSP3_RAW_MODE == 4
    run_reset_sequences(&stats);
#elif OSP3_RAW_MODE == 5
    run_overflow_geometry(&stats);
#elif OSP3_RAW_MODE == 6
    run_mutations(&stats);
#else
#error Unsupported OSP3_RAW_MODE
#endif

    printf(
        "OSP3 raw-elf mode=%u seed=%08x budget=%u cases=%u "
        "accept=%u reject=%u failures=%u digest=%u\n",
        (unsigned)OSP3_RAW_MODE,
        (unsigned)OSP3_RAW_SEED,
        (unsigned)OSP3_RAW_CASE_BUDGET,
        (unsigned)stats.cases,
        (unsigned)stats.accepts,
        (unsigned)stats.rejects,
        (unsigned)stats.failures,
        (unsigned)stats.digest
    );
    return stats.failures == 0 ? 0 : 1;
}
