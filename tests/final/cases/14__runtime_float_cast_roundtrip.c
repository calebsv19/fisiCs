#include <stdio.h>

int main(void) {
    int t1 = ((int)3.99f == 3);
    int t2 = ((int)-3.99f == -3);
    int t3 = ((float)(int)255.125f == 255.0f);
    int t4 = ((double)(int)-12.9 == -12.0);

    printf("%d %d %d %d\n", t1, t2, t3, t4);
    return 0;
}
