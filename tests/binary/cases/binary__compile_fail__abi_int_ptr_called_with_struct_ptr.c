typedef struct {
    int a;
} Box;

static int use_int_ptr(const int *p) {
    return *p;
}

int main(void) {
    Box b = {3};
    return use_int_ptr(&b);
}
