#line 18101 "virtual_wave66_function_designator_conditional_compatible_control.c"
struct SharedRoutePayload {
    int value;
};

int wave66_left(struct SharedRoutePayload *value) {
    return value ? value->value : 0;
}

int wave66_right(struct SharedRoutePayload *value) {
    return value ? value->value + 1 : 1;
}

int wave66_choose_compatible(int choose, struct SharedRoutePayload *value) {
    int (*selected)(struct SharedRoutePayload *) =
        choose ? wave66_left : wave66_right;
    return selected(value);
}

int main(void) {
    struct SharedRoutePayload value = { 4 };
    return wave66_choose_compatible(1, &value) == 4 ? 0 : 1;
}
