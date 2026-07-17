#include <stdio.h>

typedef int (*op_fn)(int, int);

static int add(int a, int b) {
    return a + b;
}

static int mul(int a, int b) {
    return a * b;
}

static int sub(int a, int b) {
    return a - b;
}

struct Dispatch {
    op_fn ops[3];
    int values[3];
};

int main(void) {
    struct Dispatch dispatch = {
        {add, mul, sub},
        {6, 7, 20}
    };

    op_fn *ops = 1 ? dispatch.ops : &dispatch.ops[1];
    op_fn selected = 0 ? ops[0] : ops[1];
    op_fn (*table_ptr)[3] = &dispatch.ops;
    op_fn through_array = (*table_ptr)[2];
    int null_check = ((0 ? dispatch.ops[0] : (op_fn)0) == (op_fn)0);

    printf("%d %d %d %d\n",
           selected(dispatch.values[0], dispatch.values[1]),
           through_array(dispatch.values[2], dispatch.values[0]),
           null_check,
           (1 ? dispatch.ops : ops)[2](dispatch.values[2], dispatch.values[1]));
    return 0;
}
