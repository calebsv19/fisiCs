#include <stdio.h>

int main(void) {
    int value = 7;
    int *ip = &value;
    void *vp = (void*)ip;
    int *ip2 = (int*)vp;
    char *cp = (char*)vp;
    int *n1 = (int*)0;
    void *n2 = (void*)0;
    int ok0 = (ip2 == &value);
    int ok1 = (cp == (char*)&value);
    int ok2 = (n1 == 0);
    int ok3 = (n2 == 0);
    printf("%d %d %d %d\n", ok0, ok1, ok2, ok3);
    return 0;
}
