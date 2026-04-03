typedef struct {
    int a;
} Box;

static int use_void_ptr(void *p) {
    return p != 0;
}

int main(void) {
    Box b = {3};
    return use_void_ptr(b);
}
