struct Wave24Payload {
    int value;
};

int probe_wave24_nested_pointer_qualifier_loss(void) {
    const struct Wave24Payload payload = {7};
    const struct Wave24Payload *read_only = &payload;
    struct Wave24Payload *writeable;
#line 12101 "virtual_lv_wave24_nested_pointer_qualifier_loss.c"
    writeable = read_only;
    return writeable->value;
}
