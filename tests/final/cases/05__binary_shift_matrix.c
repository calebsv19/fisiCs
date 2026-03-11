int main(void) {
    int signed_value = 8;
    unsigned unsigned_value = 1u;

    int arith_shift = signed_value >> 1;
    unsigned logical_shift = unsigned_value << 3;

    return arith_shift + (int)logical_shift;
}
