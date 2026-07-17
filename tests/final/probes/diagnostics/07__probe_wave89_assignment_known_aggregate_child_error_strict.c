struct Wave89Left {
    int value;
};

struct Wave89Right {
    double value;
};

int main(void) {
    struct Wave89Left wave89_target = {0};
    struct Wave89Right wave89_known_aggregate = {1.0};

    wave89_target = (wave89_missing_aggregate, wave89_known_aggregate);
    return wave89_target.value;
}
