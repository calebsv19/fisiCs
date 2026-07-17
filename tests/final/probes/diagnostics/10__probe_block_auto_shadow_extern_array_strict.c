#line 17001 "virtual_scope_block_auto_shadow_extern_array_strict.c"
int bucket10_shadow_array[2];

void bucket10_probe_shadow_conflict(void) {
    int bucket10_shadow_array[3];
    bucket10_shadow_array[0] = 1;
    {
#line 17007 "virtual_scope_block_auto_shadow_extern_array_strict.c"
        extern int bucket10_shadow_array[4];
        bucket10_shadow_array[0] = 2;
    }
}

int main(void) {
    bucket10_probe_shadow_conflict();
    return bucket10_shadow_array[0];
}
