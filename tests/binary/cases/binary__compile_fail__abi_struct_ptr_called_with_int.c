typedef struct {
    int a;
} Box;

static int use_box(const Box *b) {
    return b->a;
}

int main(void) {
    return use_box(7);
}
