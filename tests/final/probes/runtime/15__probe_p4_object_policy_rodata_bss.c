/* P4 accepted-object corpus: independent RO and zero-initialized RW sections. */

static const unsigned p4_policy_weights[] = {3u, 5u, 11u, 17u};
static unsigned p4_policy_bss_counter;

unsigned p4_object_policy_rodata_bss_entry(unsigned value) {
    unsigned weight = p4_policy_weights[value & 3u];
    p4_policy_bss_counter += weight;
    return p4_policy_bss_counter ^ (weight << 1);
}
