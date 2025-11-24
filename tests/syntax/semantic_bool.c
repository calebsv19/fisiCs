#include <stdio.h>

int main(void) {
    int flag = true;
    if (!false) {
        flag = flag && true;
    }
    return flag ? 0 : 1;
}
