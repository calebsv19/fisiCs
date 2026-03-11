typedef _Complex double cdouble_t;

cdouble_t make_value(void) {
    cdouble_t v = 2.0;
    return v;
}

int main(void) {
    cdouble_t v = make_value();
    return v == 2.0 ? 0 : 1;
}
