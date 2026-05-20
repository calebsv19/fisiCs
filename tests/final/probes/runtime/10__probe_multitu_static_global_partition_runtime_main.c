#include <stdio.h>

int state = 7;

int read_local_state(void);
void bump_local_state(int delta);

int main(void) {
    printf("%d %d ", read_local_state(), state);
    bump_local_state(4);
    printf("%d %d\n", read_local_state(), state);
    return 0;
}
