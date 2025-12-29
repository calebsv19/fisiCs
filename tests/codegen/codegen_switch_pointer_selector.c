#include <stddef.h>

int value_at(int *p) {
    switch (p) { // pointer selector should not use jump table; compare via ptrtoint chain
        case (int*)0x0: return 1;
        case (int*)0x10: return 2;
        default: return 3;
    }
}
