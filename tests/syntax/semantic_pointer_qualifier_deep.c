const int *g_const;
int *g_mut;

int main(void) {
    const int **cptrptr = 0;
    int **mptrptr = 0;
    mptrptr = cptrptr; // should reject discarding const at pointed level
    return 0;
}

int *ret_discard(const int *p) {
    return p; // should reject discarding const on return
}
