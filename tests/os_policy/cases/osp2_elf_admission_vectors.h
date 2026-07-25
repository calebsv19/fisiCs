// SPDX-License-Identifier: Apache-2.0

#ifndef OSP2_ELF_ADMISSION_VECTORS_H
#define OSP2_ELF_ADMISSION_VECTORS_H

typedef unsigned long osp2_u64;

#define OSP2_ELF_REJECT 0UL
#define OSP2_ELF_ACCEPT 1UL
#define OSP2_ELF_IGNORE 2UL
#define OSP2_ELF_VECTOR_COUNT 31UL
#define OSP2_ELF_CORPUS_ID "elf31-v1"

#define OSP2_ELF_H0 \
    0x464C457FUL, 2, 1, 2, 62, 64, 56, 64, 2, 176, 0x10000000UL
#define OSP2_ELF_S0 \
    1, 5, 0x1000, 0x10000000UL, 0x800, 0x1000, 0x4000

osp2_u64 osp2_elf_header_admit(
    osp2_u64 magic,
    osp2_u64 elf_class,
    osp2_u64 data_encoding,
    osp2_u64 elf_type,
    osp2_u64 machine,
    osp2_u64 ehsize,
    osp2_u64 phentsize,
    osp2_u64 phoff,
    osp2_u64 phnum,
    osp2_u64 image_size,
    osp2_u64 entry
);
osp2_u64 osp2_elf_load_segment_admit(
    osp2_u64 type,
    osp2_u64 flags,
    osp2_u64 offset,
    osp2_u64 vaddr,
    osp2_u64 filesz,
    osp2_u64 memsz,
    osp2_u64 image_size
);
osp2_u64 osp2_elf_load_count_admit(osp2_u64 load_count);

static osp2_u64 osp2_elf_expect(osp2_u64 actual, osp2_u64 expected) {
    return actual == expected ? 0 : 1;
}

static osp2_u64 osp2_elf_run_vectors(void) {
    osp2_u64 failures = 0;

    failures += osp2_elf_expect(
        osp2_elf_header_admit(OSP2_ELF_H0), OSP2_ELF_ACCEPT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 64, 56, 64, 8, 512, 0x106FFFFFUL
        ),
        OSP2_ELF_ACCEPT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0, 2, 1, 2, 62, 64, 56, 64, 2, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 1, 1, 2, 62, 64, 56, 64, 2, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 2, 2, 62, 64, 56, 64, 2, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 3, 62, 64, 56, 64, 2, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 3, 64, 56, 64, 2, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 63, 56, 64, 2, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 64, 55, 64, 2, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 64, 56, 64, 0, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 64, 56, 64, 9, 568, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 64, 56, 177, 1, 176, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 64, 56, 64, 2, 175, 0x10000000UL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 64, 56, 64, 2, 176, 0x0FFFFFFFUL
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_header_admit(
            0x464C457FUL, 2, 1, 2, 62, 64, 56, 64, 2, 176, 0x10700000UL
        ),
        OSP2_ELF_REJECT
    );

    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(0, 0, 0, 0, 0, 0, 0),
        OSP2_ELF_IGNORE
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(OSP2_ELF_S0), OSP2_ELF_ACCEPT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 6, 0x2000, 0x10001000UL, 0x400, 0x1000, 0x4000
        ),
        OSP2_ELF_ACCEPT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 4, 0x2000, 0x10002000UL, 0, 0x1000, 0x4000
        ),
        OSP2_ELF_ACCEPT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 5, 0x1000, 0x10000000UL, 0x1001, 0x1000, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 5, 0x4001, 0x10000001UL, 0, 1, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 5, 0x3000, 0x10000000UL, 0x1001, 0x2000, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 5, 0x0FFF, 0x0FFFFFFFUL, 0, 1, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 5, 0, 0x10700000UL, 0, 1, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 5, 0x1000, 0x10000000UL, 0, 0, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 5, 0x1000, 0x106FF000UL, 0, 0x1001, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 5, 0x1001, 0x10000000UL, 0x800, 0x1000, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 8, 0x1000, 0x10000000UL, 0x800, 0x1000, 0x4000
        ),
        OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_segment_admit(
            1, 7, 0x1000, 0x10000000UL, 0x800, 0x1000, 0x4000
        ),
        OSP2_ELF_REJECT
    );

    failures += osp2_elf_expect(
        osp2_elf_load_count_admit(0), OSP2_ELF_REJECT
    );
    failures += osp2_elf_expect(
        osp2_elf_load_count_admit(5), OSP2_ELF_REJECT
    );
    return failures;
}

#endif
