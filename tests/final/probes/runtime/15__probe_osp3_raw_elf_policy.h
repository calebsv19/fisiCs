#ifndef FISICS_OSP3_RAW_ELF_POLICY_H
#define FISICS_OSP3_RAW_ELF_POLICY_H

#include <stdint.h>

#define OSP3_RAW_MAX_LOADS 4u

enum osp3_raw_reason {
    OSP3_RAW_ACCEPT = 0,
    OSP3_RAW_REJECT_SHORT = 1,
    OSP3_RAW_REJECT_IDENT = 2,
    OSP3_RAW_REJECT_HEADER = 3,
    OSP3_RAW_REJECT_PHDR_GEOMETRY = 4,
    OSP3_RAW_REJECT_FLAGS = 5,
    OSP3_RAW_REJECT_FILE_RANGE = 6,
    OSP3_RAW_REJECT_MEMORY_RANGE = 7,
    OSP3_RAW_REJECT_CONGRUENCE = 8,
    OSP3_RAW_REJECT_LOAD_COUNT = 9,
    OSP3_RAW_REJECT_OVERLAP = 10,
    OSP3_RAW_REJECT_ENTRY = 11
};

struct osp3_raw_elf_state {
    uint32_t accepted;
    uint32_t reason;
    uint32_t load_count;
    uint32_t executable_count;
    uint32_t entry_segment;
    uint64_t min_vaddr;
    uint64_t max_vaddr;
    uint64_t file_bytes;
    uint64_t memory_bytes;
    uint64_t load_start[OSP3_RAW_MAX_LOADS];
    uint64_t load_end[OSP3_RAW_MAX_LOADS];
};

uint32_t osp3_raw_elf_admit(
    const uint8_t *image,
    uint64_t image_size,
    struct osp3_raw_elf_state *state
);

#endif
