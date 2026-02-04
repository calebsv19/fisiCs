int main(void) {
    unsigned int u = 1u;
    long l = -2;
    unsigned long ul = 3ul;
    long long ll = -4;
    unsigned long long ull = 5ull;
    unsigned long mix1 = u + l;
    unsigned long long mix2 = ul + ll + ull;
    return (int)(mix1 + mix2);
}
