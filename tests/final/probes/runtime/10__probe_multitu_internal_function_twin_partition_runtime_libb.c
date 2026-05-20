static int bucket10_private_step(int base) {
    return base * 3 - 1;
}

int bucket10_internal_function_step_b(int base) {
    return bucket10_private_step(base) + bucket10_private_step(1);
}
