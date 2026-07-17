#line 18101 "virtual_scope_wave61_block_auto_shadow_extern_array_strict.c"
int bucket10_wave61_shadow_value[3];

void bucket10_wave61_probe_shadow_conflict(void) {
    int bucket10_wave61_shadow_value[2];
    bucket10_wave61_shadow_value[0] = 1;
    {
#line 18107 "virtual_scope_wave61_block_auto_shadow_extern_array_strict.c"
        extern int bucket10_wave61_shadow_value[5];
        bucket10_wave61_shadow_value[0] = 5;
    }
}

int main(void) {
    bucket10_wave61_probe_shadow_conflict();
    return bucket10_wave61_shadow_value[0];
}
