extern int printf(const char*, ...);

typedef int wave33_count_t;

static int wave33_typedef_shadow_checksum(wave33_count_t outer) {
    int result = outer * 10;
    {
        typedef long wave33_count_t;
        wave33_count_t inner = 37;
        result += (int)inner;
    }
    return result + outer;
}

int main(void) {
    printf("%d\n", wave33_typedef_shadow_checksum(4));
    return 0;
}
