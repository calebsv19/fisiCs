#line 20801 "virtual_wave88_typedef_union_to_nested_struct_cast.c"
typedef struct Wave88Envelope {
    struct {
        int code;
        long stamp;
    } payload;
    int guard;
} Wave88Envelope;

typedef union Wave88Carrier {
    Wave88Envelope envelope;
    long words[3];
} Wave88Carrier;

static Wave88Envelope wave88_bad_cast(Wave88Carrier carrier) {
    return (Wave88Envelope)carrier;
}

int main(void) {
    Wave88Carrier carrier = {0};
    return wave88_bad_cast(carrier).guard;
}
