#line 18101 "virtual_wave66_function_designator_conditional_alpha_name.c"
struct RoutePayload {
    int outer_value;
};

int wave66_outer(struct RoutePayload *value);

int wave66_choose(int choose) {
    struct InnerPayload {
        long inner_value;
        long extra;
    };
    int wave66_inner(struct InnerPayload *value);
    (void)(choose ? wave66_outer : wave66_inner);
    return 0;
}

int main(void) {
    return wave66_choose(1);
}
