#line 20901 "virtual_wave88_include_typedef_union_to_nested_struct_cast.h"
typedef struct Wave88IncludeEnvelope {
    struct {
        int code;
        long stamp;
    } payload;
    int guard;
} Wave88IncludeEnvelope;

typedef union Wave88IncludeCarrier {
    Wave88IncludeEnvelope envelope;
    long words[3];
} Wave88IncludeCarrier;

static Wave88IncludeEnvelope wave88_include_bad_cast(Wave88IncludeCarrier carrier) {
    return (Wave88IncludeEnvelope)carrier;
}
