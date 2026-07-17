#include <stdio.h>

static int wave68_read_shared_object(int wave68_shared_object) {
    {
        extern int wave68_shared_object;
        return wave68_shared_object;
    }
}

int main(void) {
    printf("%d %d\n",
           wave68_read_shared_object(3),
           wave68_read_shared_object(99));
    return 0;
}
