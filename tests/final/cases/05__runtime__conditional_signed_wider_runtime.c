extern int printf(const char*, ...);

int main(void) {
    unsigned int u = 7u;
    long long neg = -3ll;
    long long pick_false = 0 ? u : neg;
    long long pick_true = 1 ? u : neg;
    long long seq = (0, 1) ? (u + neg) : (neg - 1);
    printf("%lld %lld %lld\n", pick_false, pick_true, seq);
    return 0;
}
