extern int printf(const char*, ...);

int main(void) {
    unsigned int one = 1u;
    unsigned int five = 5u;
    long long signed_wide = -2ll;
    long long sum = one + signed_wide;
    long long offset = five + signed_wide;
    printf("%lld %lld\n", sum, offset);
    return 0;
}
