/*
 * Compiler-side geometry mirror derived from immutable os-dev tag
 * edu-25-bounded-kernel-loading, commit
 * ebed91070912b2e40e1fa54dca22ff59697ec53a.
 *
 * EDU-25 changed stage2.asm and disk-layout tooling, not generated C. This
 * mirror probes the frozen BootInfo-v4 and bounded EDD transfer policy only.
 * BIOS calls, DAP publication, and mode transitions remain assembly-owned.
 */
typedef unsigned int edu25_u32;
typedef unsigned long long edu25_u64;

enum {
    EDU25_KERNEL_START_LBA = 18,
    EDU25_KERNEL_MAX_SECTORS = 256,
    EDU25_FILESYSTEM_START_LBA = 274,
    EDU25_KERNEL_PHYSICAL = 0x20000,
    EDU25_EDD_MAX_TRANSFER = 64,
    EDU25_SECTOR_BYTES = 512
};

int edu25_boot_geometry_valid(
    edu25_u32 kernel_start_lba,
    edu25_u32 kernel_actual_sectors,
    edu25_u32 kernel_max_sectors,
    edu25_u32 filesystem_start_lba,
    edu25_u32 kernel_load_address,
    edu25_u32 kernel_entry_address,
    edu25_u32 edd_available) {
    edu25_u64 capacity_end;
    edu25_u64 actual_end;
    if (edd_available != 1 ||
        kernel_start_lba != EDU25_KERNEL_START_LBA ||
        kernel_actual_sectors == 0 ||
        kernel_actual_sectors > EDU25_KERNEL_MAX_SECTORS ||
        kernel_max_sectors != EDU25_KERNEL_MAX_SECTORS ||
        filesystem_start_lba != EDU25_FILESYSTEM_START_LBA ||
        kernel_load_address != EDU25_KERNEL_PHYSICAL ||
        kernel_entry_address != EDU25_KERNEL_PHYSICAL) return 0;
    capacity_end =
        (edu25_u64)kernel_start_lba + (edu25_u64)kernel_max_sectors;
    actual_end =
        (edu25_u64)kernel_start_lba + (edu25_u64)kernel_actual_sectors;
    return capacity_end == filesystem_start_lba &&
           actual_end <= filesystem_start_lba;
}

edu25_u32 edu25_transfer_count(edu25_u32 actual_sectors) {
    if (actual_sectors == 0 ||
        actual_sectors > EDU25_KERNEL_MAX_SECTORS) return 0;
    return (actual_sectors + EDU25_EDD_MAX_TRANSFER - 1U) /
           EDU25_EDD_MAX_TRANSFER;
}

edu25_u32 edu25_transfer_chunk(
    edu25_u32 actual_sectors, edu25_u32 transfer_index) {
    edu25_u32 completed;
    edu25_u32 remaining;
    if (actual_sectors == 0 ||
        actual_sectors > EDU25_KERNEL_MAX_SECTORS ||
        transfer_index >= edu25_transfer_count(actual_sectors)) return 0;
    completed = transfer_index * EDU25_EDD_MAX_TRANSFER;
    remaining = actual_sectors - completed;
    return remaining > EDU25_EDD_MAX_TRANSFER ?
        EDU25_EDD_MAX_TRANSFER : remaining;
}

edu25_u64 edu25_transfer_lba(edu25_u32 transfer_index) {
    if (transfer_index >= 4) return ~0ULL;
    return (edu25_u64)EDU25_KERNEL_START_LBA +
           (edu25_u64)transfer_index * EDU25_EDD_MAX_TRANSFER;
}

edu25_u64 edu25_transfer_destination(edu25_u32 transfer_index) {
    if (transfer_index >= 4) return ~0ULL;
    return (edu25_u64)EDU25_KERNEL_PHYSICAL +
           (edu25_u64)transfer_index *
               EDU25_EDD_MAX_TRANSFER * EDU25_SECTOR_BYTES;
}

int edu25_transfer_plan_valid(
    edu25_u32 actual_sectors,
    const edu25_u32 *chunks,
    edu25_u32 chunk_count) {
    edu25_u32 index;
    edu25_u32 total = 0;
    edu25_u32 expected_count = edu25_transfer_count(actual_sectors);
    if (chunks == (const edu25_u32 *)0 ||
        expected_count == 0 ||
        chunk_count != expected_count) return 0;
    for (index = 0; index < chunk_count; index = index + 1) {
        edu25_u32 expected = edu25_transfer_chunk(actual_sectors, index);
        if (chunks[index] != expected || expected == 0) return 0;
        total = total + chunks[index];
    }
    return total == actual_sectors;
}
