#define INNER_BAD(x) ((x) + )
#define OUTER_BAD(x) INNER_BAD(x)

int main(void) {
    int x = 1;
    int y = OUTER_BAD(x);
    return y;
}
