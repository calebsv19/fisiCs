extern const unsigned axis1_wave16_epoch_weights[40];
extern const int axis1_wave16_epoch_signed_offsets[20];
extern const unsigned axis1_wave16_epoch_unsigned_offsets[20];
extern const unsigned* axis1_wave16_epoch_windows[20];
extern const unsigned** axis1_wave16_epoch_routes[10];
extern const unsigned*** axis1_wave16_epoch_plans[4];

unsigned axis1_wave16_ptrtable_epoch_window_rewind_bridge(unsigned seed) {
    unsigned acc = seed ^ axis1_wave16_epoch_weights[(seed + 3u) % 40u];
    unsigned epoch = (seed % 7u) + 3u;
    unsigned lane = 0u;

    (void)axis1_wave16_epoch_windows;
    (void)axis1_wave16_epoch_routes;

    for (; lane < 12u; ++lane) {
        const unsigned*** plan = axis1_wave16_epoch_plans[(lane + epoch) % 4u];
        const unsigned** route = plan[(lane + (seed & 3u)) % 3u];
        const unsigned* base = route[(lane + epoch) & 1u];
        const unsigned* shadow = route[((lane >> 1u) + 1u) & 1u];
        int soff = axis1_wave16_epoch_signed_offsets[(lane + epoch + seed) % 20u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave16_epoch_unsigned_offsets[(lane * 2u + epoch) % 20u];
        unsigned rewind = (epoch + uoff + shadow[0]) % 9u;

        acc = (acc * 29u) ^ (base[0] + shadow[1] + skew + rewind);
        acc += axis1_wave16_epoch_weights[(lane + uoff + rewind) % 40u];
        epoch = (epoch + rewind + (base[1] & 3u)) % 11u;
    }

    return acc ^ axis1_wave16_epoch_weights[(epoch + seed + 17u) % 40u];
}
