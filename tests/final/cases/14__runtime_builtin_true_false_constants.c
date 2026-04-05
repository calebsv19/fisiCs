#include <stdio.h>

int main(void) {
    int t = true;
    int f = false;
    int gate = true ? 10 : 1;
    int mask = false ? 100 : 3;
    int sum = gate + mask;

    printf("%d %d %d\n", t, f, sum);
    return (t == 1 && f == 0 && sum == 13) ? 0 : 7;
}
