// SPDX-License-Identifier: Apache-2.0

typedef unsigned long osp2_u64;

#define OSP2_ELF_REJECT 0UL
#define OSP2_ELF_ACCEPT 1UL
#define OSP2_ELF_IGNORE 2UL

#define OSP2_ELF_MAGIC 0x464C457FUL
#define OSP2_ELF_CLASS_64 2UL
#define OSP2_ELF_DATA_LE 1UL
#define OSP2_ELF_ET_EXEC 2UL
#define OSP2_ELF_MACHINE_X86_64 62UL
#define OSP2_ELF_EHSIZE 64UL
#define OSP2_ELF_PHENTSIZE 56UL
#define OSP2_ELF_PT_LOAD 1UL
#define OSP2_ELF_PF_X 1UL
#define OSP2_ELF_PF_W 2UL
#define OSP2_ELF_PF_R 4UL
#define OSP2_ELF_USER_MIN 0x10000000UL
#define OSP2_ELF_USER_MAX 0x10700000UL

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
) {
    if (magic != OSP2_ELF_MAGIC ||
        elf_class != OSP2_ELF_CLASS_64 ||
        data_encoding != OSP2_ELF_DATA_LE ||
        elf_type != OSP2_ELF_ET_EXEC ||
        machine != OSP2_ELF_MACHINE_X86_64 ||
        ehsize != OSP2_ELF_EHSIZE ||
        phentsize != OSP2_ELF_PHENTSIZE) {
        return OSP2_ELF_REJECT;
    }
    if (phnum == 0 || phnum > 8) {
        return OSP2_ELF_REJECT;
    }
    if (phoff > image_size ||
        phnum * OSP2_ELF_PHENTSIZE > image_size - phoff) {
        return OSP2_ELF_REJECT;
    }
    if (entry < OSP2_ELF_USER_MIN || entry >= OSP2_ELF_USER_MAX) {
        return OSP2_ELF_REJECT;
    }
    return OSP2_ELF_ACCEPT;
}

osp2_u64 osp2_elf_load_segment_admit(
    osp2_u64 type,
    osp2_u64 flags,
    osp2_u64 offset,
    osp2_u64 vaddr,
    osp2_u64 filesz,
    osp2_u64 memsz,
    osp2_u64 image_size
) {
    if (type != OSP2_ELF_PT_LOAD) {
        return OSP2_ELF_IGNORE;
    }
    if ((flags & ~(OSP2_ELF_PF_R | OSP2_ELF_PF_W | OSP2_ELF_PF_X)) != 0 ||
        (flags & (OSP2_ELF_PF_W | OSP2_ELF_PF_X)) ==
            (OSP2_ELF_PF_W | OSP2_ELF_PF_X)) {
        return OSP2_ELF_REJECT;
    }
    if (filesz > memsz || offset > image_size ||
        filesz > image_size - offset) {
        return OSP2_ELF_REJECT;
    }
    if (vaddr < OSP2_ELF_USER_MIN || vaddr >= OSP2_ELF_USER_MAX ||
        memsz == 0 || memsz > OSP2_ELF_USER_MAX - vaddr) {
        return OSP2_ELF_REJECT;
    }
    if ((vaddr & 0xFFFUL) != (offset & 0xFFFUL)) {
        return OSP2_ELF_REJECT;
    }
    return OSP2_ELF_ACCEPT;
}

osp2_u64 osp2_elf_load_count_admit(osp2_u64 load_count) {
    return load_count >= 1 && load_count <= 4
        ? OSP2_ELF_ACCEPT
        : OSP2_ELF_REJECT;
}
