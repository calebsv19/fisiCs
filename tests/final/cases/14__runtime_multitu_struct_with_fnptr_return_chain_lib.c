typedef int (*CalcFn)(int, int);

struct CalcBox {
    int bias;
    CalcFn fn;
};

static int calc_add(int x, int y) {
    return x + y;
}

static int calc_mix(int x, int y) {
    return x * 3 - y * 2;
}

static int calc_xor(int x, int y) {
    return (x ^ y) + 9;
}

struct CalcBox abi_calc_pick(int mode, int bias) {
    struct CalcBox out;
    out.bias = bias;
    if (mode == 0) {
        out.fn = calc_add;
    } else if (mode == 1) {
        out.fn = calc_mix;
    } else {
        out.fn = calc_xor;
    }
    return out;
}

struct CalcBox abi_calc_chain(struct CalcBox a, struct CalcBox b, int mode) {
    struct CalcBox out;
    out.bias = a.bias * 2 + b.bias;
    if (mode & 1) {
        out.fn = a.fn;
    } else if (mode & 2) {
        out.fn = b.fn;
    } else {
        out.fn = calc_xor;
    }
    return out;
}

int abi_calc_apply(struct CalcBox box, int x, int y) {
    return box.fn(x + box.bias, y - box.bias);
}
