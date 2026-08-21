/* P4 rejection corpus: emission is allowed, admission must reject externals. */

extern unsigned p4_unresolved_external_dependency(unsigned value);

unsigned p4_object_policy_external_entry(unsigned value) {
    return p4_unresolved_external_dependency(value) + 1u;
}
