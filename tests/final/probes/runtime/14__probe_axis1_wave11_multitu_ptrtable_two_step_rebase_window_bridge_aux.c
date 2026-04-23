extern const unsigned axis1_wave11_bridge_weights[24];
extern const unsigned* axis1_wave11_bridge_windows[12];
extern const unsigned** axis1_wave11_bridge_checkpoints[8];
extern const unsigned*** axis1_wave11_bridge_graph[4];

unsigned axis1_wave11_ptrtable_two_step_rebase_bridge(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave11_bridge_windows;
    (void)axis1_wave11_bridge_checkpoints;

    for (; lane < 7u; ++lane) {
        const unsigned*** node = axis1_wave11_bridge_graph[(seed + lane) & 3u];
        const unsigned** checkpoint = node[lane & 1u];
        const unsigned* window = checkpoint[(lane + 1u) & 1u];
        const unsigned* mirror = checkpoint[(lane + 2u) & 1u];

        acc = ((acc ^ window[(lane + seed) & 1u]) + mirror[(lane + 1u) & 1u]) * 11u;
        acc += axis1_wave11_bridge_weights[lane + 8u];
        acc ^= axis1_wave11_bridge_weights[(lane * 3u + 5u) % 24u];
    }

    return acc + axis1_wave11_bridge_weights[(seed % 8u) + 8u];
}
