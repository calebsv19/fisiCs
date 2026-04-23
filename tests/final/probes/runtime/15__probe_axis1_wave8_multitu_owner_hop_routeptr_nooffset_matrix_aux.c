extern const unsigned axis1_wave8_nooffset_weights[8];
extern const unsigned*** axis1_wave8_nooffset_plans[2];
extern unsigned (*axis1_wave8_nooffset_mix)(unsigned, unsigned);

unsigned axis1_wave8_owner_hop_nooffset_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    for (; lane < 3u; ++lane) {
        const unsigned*** plan = axis1_wave8_nooffset_plans[(seed + lane) & 1u];
        const unsigned** route = plan[lane & 1u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        acc = axis1_wave8_nooffset_mix(acc + left[0], right[1]);
        acc = axis1_wave8_nooffset_mix(acc + axis1_wave8_nooffset_weights[lane + 1u], left[1]);
    }

    return acc ^ axis1_wave8_nooffset_weights[7];
}
