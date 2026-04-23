extern const unsigned axis1_wave13_blend_weights[28];
extern const int axis1_wave13_signed_offsets[12];
extern const unsigned axis1_wave13_unsigned_offsets[12];
extern const unsigned* axis1_wave13_blend_windows[14];
extern const unsigned** axis1_wave13_blend_routes[8];

unsigned axis1_wave13_ptrtable_constexpr_offset_blend(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave13_blend_windows;

    for (; lane < 8u; ++lane) {
        const unsigned** route = axis1_wave13_blend_routes[(lane + (seed & 3u)) & 7u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        int soff = axis1_wave13_signed_offsets[(lane + seed) % 12u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave13_unsigned_offsets[(lane * 2u + 1u) % 12u];

        acc = ((acc + left[0] + right[1] + skew) ^ uoff) * 11u;
        acc += axis1_wave13_blend_weights[(lane + uoff) % 28u];
    }

    return acc ^ axis1_wave13_blend_weights[(seed + 9u) % 28u];
}
