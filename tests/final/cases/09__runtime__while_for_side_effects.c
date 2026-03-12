static int sum_for(int n) {
    int c = 0;
    int i = 0;
    for (i = 0; i < n; i = i + 1) {
        c = c + 1;
    }
    return c;
}

int main(void) {
    int a = 0;
    while (a < 3) {
        a = a + 1;
    }

    return (a == 3 && sum_for(4) == 4) ? 0 : 1;
}
