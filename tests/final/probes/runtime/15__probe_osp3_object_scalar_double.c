typedef unsigned long osp3_u64;

double osp3_object_scalar_double(
    double position,
    double velocity,
    double acceleration,
    osp3_u64 steps
) {
    osp3_u64 step = 0;
    while (step < steps) {
        velocity = velocity + acceleration;
        position = position + velocity;
        step = step + 1;
    }
    return position;
}
