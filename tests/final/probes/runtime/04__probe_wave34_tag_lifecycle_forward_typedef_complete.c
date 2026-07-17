extern int printf(const char *, ...);

struct wave34_frozen_record;
typedef struct wave34_frozen_record wave34_frozen_record_t;

static wave34_frozen_record_t *wave34_frozen_pointer_before_completion;

struct wave34_frozen_record {
    int code;
    int weights[2];
};

static int wave34_frozen_checksum(wave34_frozen_record_t *value) {
    return value->code * 100 + value->weights[0] * 10 + value->weights[1]
        + (int)sizeof(wave34_frozen_record_t);
}

int main(void) {
    wave34_frozen_record_t value = {7, {4, 9}};
    wave34_frozen_pointer_before_completion = &value;

    printf("%d\n", wave34_frozen_checksum(
        wave34_frozen_pointer_before_completion));
    return 0;
}
