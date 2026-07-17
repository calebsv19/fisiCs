typedef long wave41_bound_t;

static int wave41_bound_value(int wave41_bound_t,
                              int values[sizeof(wave41_bound_t)]) {
    return wave41_bound_t + values[0];
}

int main(void) {
    int values[1] = {35};
    return wave41_bound_value(7, values) == 42 ? 0 : 1;
}
