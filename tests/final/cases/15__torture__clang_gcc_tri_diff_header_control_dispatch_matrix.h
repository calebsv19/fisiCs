#ifndef TORTURE_CLANG_GCC_TRI_DIFF_HEADER_CONTROL_DISPATCH_MATRIX_H
#define TORTURE_CLANG_GCC_TRI_DIFF_HEADER_CONTROL_DISPATCH_MATRIX_H

typedef struct {
    unsigned op;
    int a;
    int b;
} CtrlStep;

static unsigned fold_u(unsigned acc, unsigned v) {
    acc ^= v + 0x9e3779b9u + (acc << 6u) + (acc >> 2u);
    return acc;
}

static int apply_ctrl_step(CtrlStep step, unsigned idx) {
    int out = 0;
    switch ((step.op + idx) & 3u) {
        case 0u:
            out = step.a + step.b + (int)idx;
            break;
        case 1u:
            out = step.a - step.b - (int)idx;
            break;
        case 2u:
            out = step.a * (step.b + 1);
            break;
        default:
            out = (step.a ^ step.b) + (int)(idx * 7u);
            break;
    }
    if ((idx & 1u) != 0u) {
        out ^= (int)(idx << 2u);
    }
    return out;
}

#endif
