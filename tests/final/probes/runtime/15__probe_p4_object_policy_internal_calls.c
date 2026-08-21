/* P4 accepted-object corpus: internal calls must not create external linkage. */

static unsigned p4_policy_rotate(unsigned value) {
    return (value << 7) | (value >> 25);
}

static unsigned p4_policy_mix(unsigned value) {
    return p4_policy_rotate(value ^ 0x9e3779b9u) * 33u;
}

unsigned p4_object_policy_internal_call_entry(unsigned value) {
    return p4_policy_mix(value) ^ p4_policy_rotate(value + 1u);
}
