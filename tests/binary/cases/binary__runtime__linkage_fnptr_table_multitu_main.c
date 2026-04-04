#include <stdio.h>

struct op_entry {
    const char* name;
    int (*fn)(int, int);
};

extern const struct op_entry linkage_ops[];
int linkage_ops_count(void);

int main(void) {
    int n = linkage_ops_count();
    int add_v;
    int sub_v;

    if (n != 2) {
        return 1;
    }
    add_v = linkage_ops[0].fn(7, 2);
    sub_v = linkage_ops[1].fn(7, 2);
    if (add_v != 9 || sub_v != 5) {
        return 2;
    }

    printf("linkage_fnptr_table_multitu_ok count=%d add=%d sub=%d\n", n, add_v, sub_v);
    return 0;
}
