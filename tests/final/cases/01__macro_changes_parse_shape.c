#define PICK_TERNARY(x) (x ? 10 : 20)

int main(void) {
    int x = 1;
    int y = PICK_TERNARY(x) + 2;
    return y;
}
