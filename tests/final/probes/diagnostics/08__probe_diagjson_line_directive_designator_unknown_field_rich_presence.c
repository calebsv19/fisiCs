#line 3001 "virtual_init_designator_unknown_field_probe.c"
struct Record {
    int id;
};

int main(void) {
    struct Record value = {.missing = 1};
    return value.id;
}
