int main(void) {
    long long values[8];
    long long* begin = &values[1];
    long long* end = &values[6];
    return (int)(end - begin);
}
