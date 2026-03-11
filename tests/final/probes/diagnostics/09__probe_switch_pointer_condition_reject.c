int main(void) {
    int x = 0;
    int *p = &x;
    switch (p) {
        case 0:
            return 0;
    }
    return 1;
}
