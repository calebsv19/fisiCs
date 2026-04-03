static int f(long double x) {
    return (int)x;
}

int main(void) {
    long double *p = 0;
    return f(p);
}
