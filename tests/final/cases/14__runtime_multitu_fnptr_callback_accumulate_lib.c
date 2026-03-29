typedef int (*StepFn)(int, int);

int abi_drive_steps(StepFn fn, int seed, const int* values, int count) {
    int acc = seed;
    for (int i = 0; i < count; ++i) {
        int adjusted = values[i] + (i & 1);
        acc = fn(acc, adjusted);
    }
    return acc;
}

