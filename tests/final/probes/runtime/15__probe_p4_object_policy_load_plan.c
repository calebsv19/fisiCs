/*
 * Synthetic P4 intake canary.  This is intentionally not an EDU-50 source
 * snapshot: it exercises only the compiler-owned object admission shape.
 */

/* Retained solely to require a non-executable writable section. */
static unsigned p4_data_marker = 7u;

unsigned p4_load_plan_policy_fold(unsigned value) {
    value ^= value >> 13;
    value *= 0x45d9f3bu;
    return value ^ (value >> 16);
}

unsigned p4_load_plan_policy_admit(unsigned section_count,
                                   unsigned relocation_count,
                                   unsigned image_pages,
                                   unsigned has_undefined_symbols) {
    unsigned state = p4_data_marker + section_count + relocation_count + image_pages;

    if (section_count == 0u || section_count > 32u) {
        return 0u;
    }
    if (relocation_count > 512u || image_pages == 0u || image_pages > 64u) {
        return 0u;
    }
    if (has_undefined_symbols != 0u) {
        return 0u;
    }

    p4_data_marker = p4_load_plan_policy_fold(state);
    return p4_data_marker | 1u;
}
