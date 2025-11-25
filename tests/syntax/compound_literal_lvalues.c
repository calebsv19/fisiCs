struct S { int x; };

void ok(struct S s) {
    (struct S){ .x = 1 }.x = 2;
}

static struct S bad = (struct S){ .x = 3 };
struct S bad2 = (struct S){ .x = 4 };
