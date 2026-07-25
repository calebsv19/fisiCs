typedef unsigned long osp_u64;

double osp_scalar_zero_init(double position,
                            double velocity,
                            double acceleration,
                            osp_u64 steps) {
    osp_u64 step = 0;
    double correction = 0;
    int* optional_value = 0;

    while (step < steps) {
        velocity += acceleration;
        position += velocity + correction;
        step++;
    }

    return optional_value ? 0 : position;
}
