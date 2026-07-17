struct NextAbiLarge {
    int lane[10];
    int tag;
    int salt;
};

typedef struct NextAbiLarge (*NextAbiLargeFn)(struct NextAbiLarge, int);

static struct NextAbiLarge nextbatch_large_twist(struct NextAbiLarge in, int step) {
    struct NextAbiLarge out;
    int i;

    for (i = 0; i < 10; ++i) {
        out.lane[i] = in.lane[i] * ((step % 3) + 2) - in.lane[(9 - i + step) % 10];
    }
    out.tag = in.tag + step * 13 + out.lane[step % 10];
    out.salt = in.salt ^ (out.lane[(step + 5) % 10] + step * 29);
    return out;
}

static struct NextAbiLarge nextbatch_large_blend(struct NextAbiLarge in, int step) {
    struct NextAbiLarge out;
    int i;

    for (i = 0; i < 10; ++i) {
        out.lane[i] = in.lane[(i + 3) % 10] + in.lane[(i + step) % 10] - step * 5;
    }
    out.tag = in.tag ^ (step * 17 + out.lane[(step + 1) % 10]);
    out.salt = in.salt + out.lane[(step + 7) % 10] - step * 11;
    return out;
}

struct NextAbiLarge nextbatch_large_seed(int base) {
    struct NextAbiLarge out;
    int i;

    for (i = 0; i < 10; ++i) {
        out.lane[i] = base + i * 7 - (i % 3) * 5;
    }
    out.tag = base * 19 + 5;
    out.salt = base * 31 - 9;
    return out;
}

struct NextAbiLarge nextbatch_large_bridge(struct NextAbiLarge in, NextAbiLargeFn cb, int rounds) {
    NextAbiLargeFn table[3];
    struct NextAbiLarge cur = in;
    int i;

    table[0] = nextbatch_large_twist;
    table[1] = cb;
    table[2] = nextbatch_large_blend;

    for (i = 0; i < rounds; ++i) {
        NextAbiLargeFn selected = table[(i + cur.tag) % 3];
        cur = selected(cur, i + 1);
    }

    return cur;
}

unsigned nextbatch_large_fold(struct NextAbiLarge value) {
    unsigned acc = (unsigned)(value.tag * 2654435761u) ^ (unsigned)value.salt;
    int i;

    for (i = 0; i < 10; ++i) {
        acc ^= (unsigned)(value.lane[i] + i * 101);
        acc = (acc << 5) | (acc >> 27);
        acc += (unsigned)(value.lane[(i + 4) % 10] * 17);
    }
    return acc ^ (acc >> 16);
}
