extern const unsigned axis1_wave20_route_weights[56];
extern const int axis1_wave20_route_signed_offsets[28];
extern const unsigned axis1_wave20_route_unsigned_offsets[28];
extern const unsigned* axis1_wave20_route_windows[28];
extern const unsigned** axis1_wave20_route_routes[14];
extern const unsigned*** axis1_wave20_route_plans[4];

unsigned axis1_wave20_ptrtable_checkpoint_route_replay_fallback_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave20_route_weights[(seed + 5u) % 56u];
    unsigned checkpoint = (seed % 15u) + 1u;
    unsigned replay = (seed % 7u) + 2u;
    unsigned lane = 0u;

    for (; lane < 14u; ++lane) {
        const unsigned*** plan = axis1_wave20_route_plans[(lane + checkpoint) % 4u];
        const unsigned** route = plan[(lane + replay) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* mirror = route[((lane + replay + 1u) >> 1u) & 1u];
        int soff = axis1_wave20_route_signed_offsets[(lane + checkpoint + replay) % 28u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave20_route_unsigned_offsets[(lane * 2u + checkpoint + replay) % 28u];

        acc = (acc * 47u) ^ (base[0] + mirror[1] + skew + checkpoint);
        acc += axis1_wave20_route_weights[(lane + uoff + mirror[0]) % 56u];
        replay = (replay + (base[1] & 3u) + (mirror[0] & 1u)) % 19u;
        checkpoint = (checkpoint + replay + (lane & 3u)) % 23u;
    }

    return acc ^ axis1_wave20_route_weights[(seed + checkpoint + replay + 21u) % 56u];
}
