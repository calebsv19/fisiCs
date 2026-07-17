#include <stdio.h>

typedef int Wave89Value;

static Wave89Value wave89_object = 37;
static Wave89Value *const wave89_pointers[1] = {
    &wave89_object,
};

int main(void) {
    Wave89Value *wave89_pointer = wave89_pointers[0];
    int same_object = wave89_pointer == &wave89_object;

    printf("%d %d\n", *wave89_pointer, same_object);
    return (*wave89_pointer == 37 && same_object) ? 0 : 89;
}
