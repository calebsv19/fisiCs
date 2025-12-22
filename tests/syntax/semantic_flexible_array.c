// Flexible array member semantics.

struct FlexOK { int count; int data[]; };
struct FlexBad { int count; int data[]; int tail; };
union FlexUnion { int count; int data[]; };

int consume(struct FlexOK* f) {
    return f ? f->count : 0;
}

int demo(void) {
    struct FlexOK local;          // invalid: incomplete type
    return sizeof(struct FlexOK); // invalid: incomplete for sizeof
}
