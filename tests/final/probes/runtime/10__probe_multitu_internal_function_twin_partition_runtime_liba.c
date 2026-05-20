static int bucket10_private_step(int base) {
    return base * 2 + 3;
}

int bucket10_internal_function_step_a(int base) {
    return bucket10_private_step(base) + bucket10_private_step(1);
}
