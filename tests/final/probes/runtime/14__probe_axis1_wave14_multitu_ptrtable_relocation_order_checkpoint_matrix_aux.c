extern const unsigned axis1_wave14_weights[30];
extern const int axis1_wave14_signed_offsets[14];
extern const unsigned axis1_wave14_unsigned_offsets[14];
extern const unsigned* axis1_wave14_windows[15];
extern const unsigned** axis1_wave14_routes[8];
extern const unsigned*** axis1_wave14_checkpoints[6];

unsigned axis1_wave14_ptrtable_relocation_order_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave14_windows;

    for (; lane < 9u; ++lane) {
        const unsigned*** checkpoint = axis1_wave14_checkpoints[(lane + (seed & 1u)) % 6u];
        const unsigned** route = checkpoint[lane & 1u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        int soff = axis1_wave14_signed_offsets[(lane + seed) % 14u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave14_unsigned_offsets[(lane * 2u + 1u) % 14u];

        acc = ((acc + left[0] + right[1] + skew) ^ uoff) * 13u;
        acc += axis1_wave14_weights[(lane + uoff) % 30u];
        acc ^= axis1_wave14_weights[(lane * 3u + skew + 7u) % 30u];
    }

    return acc ^ axis1_wave14_weights[(seed + 13u) % 30u];
}
