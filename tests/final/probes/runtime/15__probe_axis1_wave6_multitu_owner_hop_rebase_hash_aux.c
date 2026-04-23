extern const unsigned axis1_wave6_owner_weights[4];
extern const unsigned* axis1_wave6_owner_hops[2];
extern unsigned (*axis1_wave6_owner_mix_ptr)(unsigned, unsigned);

unsigned axis1_wave6_owner_hop_fold(unsigned seed) {
    unsigned acc = seed;
    int i = 0;
    for (; i < 2; ++i) {
        const unsigned* hop = axis1_wave6_owner_hops[i];
        acc = axis1_wave6_owner_mix_ptr(acc + axis1_wave6_owner_weights[i], hop[0]);
        acc = axis1_wave6_owner_mix_ptr(acc + axis1_wave6_owner_weights[i + 1], hop[1]);
    }
    return acc;
}
