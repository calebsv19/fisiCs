#include "recursive_guard_a.h"

int main(void) {
    return ((PP_RECUR_A + PP_RECUR_B) == 3) ? 0 : 1;
}
