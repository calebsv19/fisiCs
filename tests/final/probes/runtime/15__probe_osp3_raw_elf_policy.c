#include "15__probe_osp3_raw_elf_policy.h"

#define OSP3_ELF_EHSIZE 64u
#define OSP3_ELF_PHENTSIZE 56u
#define OSP3_ELF_PT_LOAD 1u
#define OSP3_ELF_PF_X 1u
#define OSP3_ELF_PF_W 2u
#define OSP3_ELF_PF_R 4u
#define OSP3_ELF_USER_MIN 0x10000000ULL
#define OSP3_ELF_USER_MAX 0x10700000ULL

static uint16_t osp3_read_u16(const uint8_t *bytes) {
    return (uint16_t)(
        (uint16_t)bytes[0] |
        ((uint16_t)bytes[1] << 8)
    );
}

static uint32_t osp3_read_u32(const uint8_t *bytes) {
    return (
        (uint32_t)bytes[0] |
        ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) |
        ((uint32_t)bytes[3] << 24)
    );
}

static uint64_t osp3_read_u64(const uint8_t *bytes) {
    return (
        (uint64_t)bytes[0] |
        ((uint64_t)bytes[1] << 8) |
        ((uint64_t)bytes[2] << 16) |
        ((uint64_t)bytes[3] << 24) |
        ((uint64_t)bytes[4] << 32) |
        ((uint64_t)bytes[5] << 40) |
        ((uint64_t)bytes[6] << 48) |
        ((uint64_t)bytes[7] << 56)
    );
}

static void osp3_state_reset(struct osp3_raw_elf_state *state) {
    uint32_t i;
    state->accepted = 0;
    state->reason = OSP3_RAW_ACCEPT;
    state->load_count = 0;
    state->executable_count = 0;
    state->entry_segment = UINT32_MAX;
    state->min_vaddr = 0;
    state->max_vaddr = 0;
    state->file_bytes = 0;
    state->memory_bytes = 0;
    for (i = 0; i < OSP3_RAW_MAX_LOADS; ++i) {
        state->load_start[i] = 0;
        state->load_end[i] = 0;
    }
}

static uint32_t osp3_reject(
    struct osp3_raw_elf_state *state,
    uint32_t reason
) {
    osp3_state_reset(state);
    state->reason = reason;
    return reason;
}

static int osp3_ranges_overlap(
    uint64_t left_start,
    uint64_t left_end,
    uint64_t right_start,
    uint64_t right_end
) {
    return left_start < right_end && right_start < left_end;
}

uint32_t osp3_raw_elf_admit(
    const uint8_t *image,
    uint64_t image_size,
    struct osp3_raw_elf_state *state
) {
    uint64_t entry;
    uint64_t phoff;
    uint16_t phentsize;
    uint16_t phnum;
    uint32_t i;

    osp3_state_reset(state);
    if (image_size < OSP3_ELF_EHSIZE) {
        return osp3_reject(state, OSP3_RAW_REJECT_SHORT);
    }
    if (
        image[0] != 0x7f ||
        image[1] != 'E' ||
        image[2] != 'L' ||
        image[3] != 'F' ||
        image[4] != 2 ||
        image[5] != 1 ||
        image[6] != 1 ||
        image[7] != 0 ||
        image[8] != 0
    ) {
        return osp3_reject(state, OSP3_RAW_REJECT_IDENT);
    }
    if (
        osp3_read_u16(image + 16) != 2 ||
        osp3_read_u16(image + 18) != 62 ||
        osp3_read_u32(image + 20) != 1 ||
        osp3_read_u16(image + 52) != OSP3_ELF_EHSIZE
    ) {
        return osp3_reject(state, OSP3_RAW_REJECT_HEADER);
    }

    entry = osp3_read_u64(image + 24);
    phoff = osp3_read_u64(image + 32);
    phentsize = osp3_read_u16(image + 54);
    phnum = osp3_read_u16(image + 56);
    if (
        phentsize != OSP3_ELF_PHENTSIZE ||
        phnum == 0 ||
        phnum > 8 ||
        phoff > image_size ||
        (uint64_t)phnum > (image_size - phoff) / OSP3_ELF_PHENTSIZE
    ) {
        return osp3_reject(state, OSP3_RAW_REJECT_PHDR_GEOMETRY);
    }

    for (i = 0; i < (uint32_t)phnum; ++i) {
        const uint8_t *ph = image + phoff + (uint64_t)i * OSP3_ELF_PHENTSIZE;
        uint32_t type = osp3_read_u32(ph);
        uint32_t flags;
        uint64_t offset;
        uint64_t vaddr;
        uint64_t filesz;
        uint64_t memsz;
        uint64_t vend;
        uint64_t file_end;
        uint32_t prior;

        if (type != OSP3_ELF_PT_LOAD) {
            continue;
        }
        if (state->load_count >= OSP3_RAW_MAX_LOADS) {
            return osp3_reject(state, OSP3_RAW_REJECT_LOAD_COUNT);
        }
        flags = osp3_read_u32(ph + 4);
        offset = osp3_read_u64(ph + 8);
        vaddr = osp3_read_u64(ph + 16);
        filesz = osp3_read_u64(ph + 32);
        memsz = osp3_read_u64(ph + 40);

        if (
            (flags & ~(OSP3_ELF_PF_R | OSP3_ELF_PF_W | OSP3_ELF_PF_X)) != 0 ||
            (flags & (OSP3_ELF_PF_W | OSP3_ELF_PF_X)) ==
                (OSP3_ELF_PF_W | OSP3_ELF_PF_X)
        ) {
            return osp3_reject(state, OSP3_RAW_REJECT_FLAGS);
        }
        if (
            filesz > memsz ||
            offset > image_size ||
            filesz > image_size - offset
        ) {
            return osp3_reject(state, OSP3_RAW_REJECT_FILE_RANGE);
        }
        if (
            vaddr < OSP3_ELF_USER_MIN ||
            vaddr >= OSP3_ELF_USER_MAX ||
            memsz == 0 ||
            memsz > OSP3_ELF_USER_MAX - vaddr
        ) {
            return osp3_reject(state, OSP3_RAW_REJECT_MEMORY_RANGE);
        }
        if ((vaddr & 0xfffULL) != (offset & 0xfffULL)) {
            return osp3_reject(state, OSP3_RAW_REJECT_CONGRUENCE);
        }

        vend = vaddr + memsz;
        file_end = vaddr + filesz;
        for (prior = 0; prior < state->load_count; ++prior) {
            if (
                osp3_ranges_overlap(
                    vaddr,
                    vend,
                    state->load_start[prior],
                    state->load_end[prior]
                )
            ) {
                return osp3_reject(state, OSP3_RAW_REJECT_OVERLAP);
            }
        }

        state->load_start[state->load_count] = vaddr;
        state->load_end[state->load_count] = vend;
        if (state->load_count == 0 || vaddr < state->min_vaddr) {
            state->min_vaddr = vaddr;
        }
        if (state->load_count == 0 || vend > state->max_vaddr) {
            state->max_vaddr = vend;
        }
        state->file_bytes += filesz;
        state->memory_bytes += memsz;
        if ((flags & OSP3_ELF_PF_X) != 0) {
            ++state->executable_count;
            if (entry >= vaddr && entry < file_end) {
                if (state->entry_segment != UINT32_MAX) {
                    return osp3_reject(state, OSP3_RAW_REJECT_ENTRY);
                }
                state->entry_segment = state->load_count;
            }
        }
        ++state->load_count;
    }

    if (state->load_count == 0 || state->load_count > OSP3_RAW_MAX_LOADS) {
        return osp3_reject(state, OSP3_RAW_REJECT_LOAD_COUNT);
    }
    if (
        state->executable_count == 0 ||
        state->entry_segment == UINT32_MAX
    ) {
        return osp3_reject(state, OSP3_RAW_REJECT_ENTRY);
    }
    state->accepted = 1;
    state->reason = OSP3_RAW_ACCEPT;
    return OSP3_RAW_ACCEPT;
}
