#include <stdio.h>

struct Wave90Row {
    int header;
    int cells[3];
};

static struct Wave90Row wave90_rows[2] = {
    { 5, { 7, 11, 13 } },
    { 17, { 19, 23, 29 } },
};

static void *wave90_member_address =
    (void *)&wave90_rows[1].cells[2];
static int *wave90_byte_offset_address =
    (int *)((char *)&wave90_rows[0].cells[0] + sizeof(int) * 2);
static int *wave90_selected_cast_address =
    1 ? (int *)(void *)&wave90_rows[1].cells[1] : (int *)0;

int main(void) {
    int *member = (int *)wave90_member_address;
    int member_same = member == &wave90_rows[1].cells[2];
    int offset_same = wave90_byte_offset_address == &wave90_rows[0].cells[2];
    int selected_same = wave90_selected_cast_address == &wave90_rows[1].cells[1];

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
