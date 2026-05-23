#include <stdio.h>

extern unsigned ptr_alias_owner_replay_mesh(unsigned seed);

int main(void) {
    printf("%u %u\n",
           ptr_alias_owner_replay_mesh(55u),
           ptr_alias_owner_replay_mesh(109u));
    return 0;
}
