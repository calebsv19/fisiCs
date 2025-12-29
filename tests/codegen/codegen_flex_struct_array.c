// Regression: flexible array of structs should be addressable for reads/writes.

struct Entry {
    short a;
    short b;
};

struct Rec {
    int count;
    struct Entry entries[];
};

static int exercise(void) {
    char buf[sizeof(struct Rec) + 2 * sizeof(struct Entry)];
    struct Rec* r = (struct Rec*)buf;
    r->count = 2;
    r->entries[0].a = 1;
    r->entries[0].b = 2;
    r->entries[1].a = 4;
    r->entries[1].b = 8;
    int sum0 = r->entries[0].a + r->entries[0].b;
    int sum1 = r->entries[1].a + r->entries[1].b;
    return sum0 + sum1; // expect 1+2+4+8 = 15
}

int main(void) {
    return exercise();
}
