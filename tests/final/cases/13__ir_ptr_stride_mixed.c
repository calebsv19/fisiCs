int main(void) {
    int ints[8];
    int* pi = &ints[1];
    int* qi = &ints[6];

    long long longs[8];
    long long* pl = &longs[2];
    long long* ql = &longs[7];

    return (int)((qi - pi) * 10 + (ql - pl));
}
