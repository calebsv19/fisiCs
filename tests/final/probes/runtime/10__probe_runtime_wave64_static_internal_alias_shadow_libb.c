extern int bucket10_wave64_alias_shared;

static int bucket10_wave64_alias = 200;

int bucket10_wave64_alias_libb(int step) {
    static int local = 5;

    local += step;
    bucket10_wave64_alias -= local;
    bucket10_wave64_alias_shared += local;
    {
        int bucket10_wave64_alias = local + step;
        int file_scope_alias = 189;
        return file_scope_alias + bucket10_wave64_alias_shared +
               bucket10_wave64_alias;
    }
}

int bucket10_wave64_alias_libb_peek(void) {
    return bucket10_wave64_alias;
}
