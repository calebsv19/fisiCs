#include <stdio.h>

extern unsigned owner_window_borrow_replay_mesh(unsigned seed);

int main(void) {
    printf("%u %u\n",
           owner_window_borrow_replay_mesh(37u),
           owner_window_borrow_replay_mesh(101u));
    return 0;
}
