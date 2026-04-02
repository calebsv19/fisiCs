#include <stdio.h>

int main(void) {
    int values[6] = {3, 5, 7, 11, 13, 17};
    const int *cp = &values[1];
    int *p = &values[4];
    const void *cv = (const void*)cp;
    void *v = (void*)p;
    const int *rp = (const int*)v;

    int lt = (cp < rp);
    int le = (cp <= rp);
    int gt = (rp > cp);
    int ge = (rp >= cp);
    int ne = (cp != rp);
    int eq = ((const int*)cv == cp);
    int delta = (int)(rp - cp);

    printf("%d %d %d %d %d %d %d\n", lt, le, gt, ge, ne, eq, delta);
    return 0;
}
