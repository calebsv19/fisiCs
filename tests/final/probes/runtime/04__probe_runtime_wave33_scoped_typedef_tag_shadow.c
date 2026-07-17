extern int printf(const char*, ...);

struct wave33_record {
    int left;
    int right;
};
typedef struct wave33_record wave33_record_t;

static int wave33_shadow_checksum(wave33_record_t outer) {
    int result = outer.left * 10 + outer.right;
    {
        struct wave33_record {
            int total;
        } inner = {37};
        typedef struct wave33_record wave33_record_t;
        wave33_record_t copy = inner;
        result += copy.total;
    }
    return result + outer.left - outer.right;
}

int main(void) {
    wave33_record_t outer = {4, 9};
    printf("%d\n", wave33_shadow_checksum(outer));
    return 0;
}
