#include <stdio.h>

int bucket10_wave64_alias_shared = 11;

int bucket10_wave64_alias_liba(int step);
int bucket10_wave64_alias_libb(int step);
int bucket10_wave64_alias_liba_peek(void);
int bucket10_wave64_alias_libb_peek(void);

static int bucket10_wave64_alias_main(int step) {
    extern int bucket10_wave64_alias_shared;
    static int bucket10_wave64_alias = 30;

    bucket10_wave64_alias += step;
    bucket10_wave64_alias_shared += bucket10_wave64_alias;
    {
        int bucket10_wave64_alias = bucket10_wave64_alias_shared;
        return bucket10_wave64_alias + bucket10_wave64_alias_shared;
    }
}

int main(void) {
    printf("%d %d %d %d %d %d\n",
           bucket10_wave64_alias_liba(4),
           bucket10_wave64_alias_main(2),
           bucket10_wave64_alias_libb(6),
           bucket10_wave64_alias_shared,
           bucket10_wave64_alias_liba_peek(),
           bucket10_wave64_alias_libb_peek());
    return 0;
}
