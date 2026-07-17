extern int bucket10_wave64_alias_shared;

static int bucket10_wave64_alias = 100;

int bucket10_wave64_alias_liba(int step) {
    static int local = 3;

    local += step;
    bucket10_wave64_alias += local;
    bucket10_wave64_alias_shared += step;
    {
        int bucket10_wave64_alias = local + step;
        int file_scope_alias = 107;
        return bucket10_wave64_alias + bucket10_wave64_alias_shared +
               file_scope_alias;
    }
}

int bucket10_wave64_alias_liba_peek(void) {
    return bucket10_wave64_alias;
}
