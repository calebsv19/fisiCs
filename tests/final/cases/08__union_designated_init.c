union U {
    int i;
    char c[4];
};

union U u = { .c = {1, 2, 3, 4} };

int main(void) {
    return u.c[0];
}
