#line 34001 "wave34_const_object_ice.c"
static const int wave34_limit = 3;
enum { WAVE34_ENUM_LIMIT = wave34_limit };

int wave34_select(int value) {
    switch (value) {
        case wave34_limit:
            return WAVE34_ENUM_LIMIT;
        default:
            return 0;
    }
}

int main(void) {
    return wave34_select(3);
}
