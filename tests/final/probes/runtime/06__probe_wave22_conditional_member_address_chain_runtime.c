#include <stdio.h>

struct Wave22AddressSlot {
    int value;
    int lane[3];
};

struct Wave22AddressBox {
    struct Wave22AddressSlot slots[2];
    int adjust;
};

static int wave22_conditional_member_address_chain(void) {
    struct Wave22AddressBox boxes[2] = {
        {{{5, {7, 11, 13}}, {17, {19, 23, 29}}}, 31},
        {{{37, {41, 43, 47}}, {53, {59, 61, 67}}}, 71},
    };

    int pick = boxes[1].slots[1].lane[0] > boxes[0].slots[1].lane[2];
    struct Wave22AddressBox *selected = pick ? &boxes[1] : &boxes[0];
    int *slot = &selected->slots[pick ? 1 : 0].lane[pick ? 0 : 2];
    int *value = &selected->slots[pick ? 1 : 0].value;

    *slot += selected->adjust;
    *value += *slot;
    selected->adjust += *value;

    return *slot + *value + selected->adjust;
}

int main(void) {
    printf("%d\n", wave22_conditional_member_address_chain());
    return 0;
}
