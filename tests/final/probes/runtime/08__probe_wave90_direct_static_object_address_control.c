#include <stdio.h>

static int wave90_member_value = 29;
static int wave90_offset_value = 13;
static int wave90_selected_value = 23;

static void *wave90_member_address = &wave90_member_value;
static int *wave90_byte_offset_address = &wave90_offset_value;
static int *wave90_selected_cast_address = &wave90_selected_value;

int main(void) {
    int *member = (int *)wave90_member_address;
    int member_same = member == &wave90_member_value;
    int offset_same = wave90_byte_offset_address == &wave90_offset_value;
    int selected_same = wave90_selected_cast_address == &wave90_selected_value;

    printf("%d %d %d %d %d %d\n",
           *member,
           *wave90_byte_offset_address,
           *wave90_selected_cast_address,
           member_same,
           offset_same,
           selected_same);
    return (*member == 29 &&
            *wave90_byte_offset_address == 13 &&
            *wave90_selected_cast_address == 23 &&
            member_same && offset_same && selected_same) ? 0 : 90;
}
