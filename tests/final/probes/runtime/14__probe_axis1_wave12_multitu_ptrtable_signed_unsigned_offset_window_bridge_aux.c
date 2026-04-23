extern const unsigned axis1_wave12_bridge_weights[26];
extern const int axis1_wave12_bridge_signed_offsets[12];
extern const unsigned axis1_wave12_bridge_unsigned_offsets[12];
extern const unsigned* axis1_wave12_bridge_windows[13];
extern const unsigned** axis1_wave12_bridge_checkpoints[8];
extern const unsigned*** axis1_wave12_bridge_graph[4];

unsigned axis1_wave12_ptrtable_signed_unsigned_bridge(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave12_bridge_windows;
    (void)axis1_wave12_bridge_checkpoints;

    for (; lane < 8u; ++lane) {
        const unsigned*** node = axis1_wave12_bridge_graph[(seed + lane) & 3u];
        const unsigned** checkpoint = node[lane & 1u];
        const unsigned* left = checkpoint[(lane + 1u) & 1u];
        const unsigned* right = checkpoint[(lane + 2u) & 1u];
        int soff = axis1_wave12_bridge_signed_offsets[(lane + seed) % 12u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave12_bridge_unsigned_offsets[(lane * 3u + 2u) % 12u];

        acc = ((acc + left[0] + right[1] + skew) ^ uoff) * 13u;
        acc += axis1_wave12_bridge_weights[(lane + uoff) % 26u];
        acc ^= axis1_wave12_bridge_weights[(lane * 2u + skew + 5u) % 26u];
    }

    return acc + axis1_wave12_bridge_weights[(seed + 9u) % 26u];
}
