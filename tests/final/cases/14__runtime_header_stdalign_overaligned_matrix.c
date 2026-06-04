#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    struct Wave15OveralignedBox {
        alignas(16) unsigned char bytes[16];
        int tail;
    };
    static struct Wave15OveralignedBox g_wave15_box = {{0}, 7};
    size_t box_align = _Alignof(struct Wave15OveralignedBox);
    size_t tail_offset = offsetof(struct Wave15OveralignedBox, tail);
    unsigned long long addr_mod = (unsigned long long)(((uintptr_t)(void *)&g_wave15_box.bytes[0]) % 16u);

    if (box_align < 16u) {
        return 1;
    }
    if ((tail_offset % 16u) != 0u) {
        return 2;
    }
    if (addr_mod != 0u) {
        return 3;
    }

    printf(
        "%llu %llu %llu %d %d\n",
        (unsigned long long)box_align,
        (unsigned long long)tail_offset,
        addr_mod,
        __alignas_is_defined,
        __alignof_is_defined
    );
    return 0;
}
