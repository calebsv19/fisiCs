#include <stdio.h>

typedef int (*UnaryFn)(int);

typedef struct Entry {
    unsigned id;
    const char* name;
    UnaryFn fn;
    const int* vals;
    unsigned count;
} Entry;

static int add_one(int x) {
    return x + 1;
}

static const int kValsA[] = {3, 4};
static const int kValsB[] = {7};

#define ENTRY(id, name, fn, vals, count) {id, name, fn, vals, count}

static Entry kTable[] = {
    ENTRY(11u, "alpha", add_one, kValsA, 2u),
    ENTRY(22u, "beta", add_one, kValsB, 1u),
};

#undef ENTRY

static const int kTableCount = (int)(sizeof(kTable) / sizeof(kTable[0]));

int main(void) {
    if (kTableCount != 2) {
        printf("bad-count:%d\n", kTableCount);
        return 1;
    }
    if (!kTable[0].name || !kTable[1].name || !kTable[0].fn || !kTable[1].vals) {
        puts("bad-null");
        return 1;
    }

    int sum = 0;
    sum += kTable[0].fn(5);
    sum += kTable[1].fn(8);
    sum += kTable[0].vals[0] + kTable[0].vals[1] + kTable[1].vals[0];
    sum += (int)kTable[0].id + (int)kTable[1].id;
    sum += (kTable[0].name[0] == 'a');
    sum += (kTable[1].name[0] == 'b');

    if (sum != 64) {
        printf("bad-sum:%d\n", sum);
        return 1;
    }

    printf("ok %d\n", sum);
    return 0;
}
