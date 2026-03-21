int main(void) {
    unsigned int u = 1u;
    int s = -1;
    int a = (u < s);
    int b = (s < u);
    return a * 10 + b;
}
