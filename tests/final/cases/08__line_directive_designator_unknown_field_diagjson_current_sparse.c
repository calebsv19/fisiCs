#line 2801 "virtual_init_designator_unknown_field_case.c"
struct Record {
    int id;
};

int main(void) {
    struct Record value = {.missing = 1};
    return value.id;
}
