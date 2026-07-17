static int wave69_route_exact(double value);

static int wave69_call_exact(int wave69_route_exact) {
    {
        extern int wave69_route_exact(double value);
        return wave69_route_exact(41.0);
    }
}

static int wave69_route_exact(double value) {
    return (int)value + 1;
}

int main(void) {
    return wave69_call_exact(7) == 42 ? 0 : 1;
}
