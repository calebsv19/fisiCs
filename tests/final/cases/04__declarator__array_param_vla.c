int sum(int n, int values[n]) {
    int i;
    int acc = 0;
    for (i = 0; i < n; ++i) {
        acc += values[i];
    }
    return acc;
}

int main(void) {
    int vals[3] = {1, 2, 3};
    return sum(3, vals) - 6;
}
