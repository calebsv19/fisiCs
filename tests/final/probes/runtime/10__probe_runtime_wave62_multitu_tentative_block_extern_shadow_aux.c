int bucket10_wave62_shared[4];

int bucket10_wave62_aux_total(void) {
    extern int bucket10_wave62_shared[];

    return bucket10_wave62_shared[0] + bucket10_wave62_shared[1] +
           bucket10_wave62_shared[2] + bucket10_wave62_shared[3];
}
