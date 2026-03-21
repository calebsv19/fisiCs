int main(void) {
    long long values[4];
    long long* p = values;
    long long* q = p + 3;
    return (int)(q - p);
}
