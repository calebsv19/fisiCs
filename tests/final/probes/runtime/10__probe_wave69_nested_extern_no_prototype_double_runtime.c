static int wave69_route_double(double value);

static int wave69_call_double(int wave69_route_double) {
    {
        extern int wave69_route_double();
        return wave69_route_double(41.0);
    }
}

static int wave69_route_double(double value) {
    return (int)value + 1;
}

int main(void) {
    return wave69_call_double(7) == 42 ? 0 : 1;
}
