extern int printf(const char *, ...);

struct wave34_complete_record;

static struct wave34_complete_record *wave34_complete_pointer_before_completion;
static struct wave34_complete_record *wave34_complete_identity(
    struct wave34_complete_record *value);

struct wave34_complete_record {
    int left;
    int right;
};

typedef struct wave34_complete_record wave34_complete_record_t;

static struct wave34_complete_record *wave34_complete_identity(
    struct wave34_complete_record *value) {
    return value;
}

int main(void) {
    wave34_complete_record_t value = {6, 13};
    wave34_complete_pointer_before_completion = &value;

    struct wave34_complete_record *selected =
        wave34_complete_identity(wave34_complete_pointer_before_completion);
    int checksum = selected->left * 100 + selected->right * 3
        + (int)sizeof(wave34_complete_record_t);

    printf("%d\n", checksum);
    return 0;
}
