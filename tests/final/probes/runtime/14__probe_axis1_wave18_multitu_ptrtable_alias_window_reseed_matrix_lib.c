const unsigned axis1_wave18_window_weights[50] = {
    13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u,
    53u, 57u, 61u, 65u, 69u, 73u, 77u, 81u, 85u, 89u,
    93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u, 125u, 129u,
    133u, 137u, 141u, 145u, 149u, 153u, 157u, 161u, 165u, 169u,
    173u, 177u, 181u, 185u, 189u, 193u, 197u, 201u, 205u, 209u,
};

const unsigned axis1_wave18_window_lane_masks[12] = {
    3u, 6u, 9u, 5u, 10u, 12u, 7u, 11u, 13u, 14u, 8u, 15u,
};

const int axis1_wave18_window_signed_offsets[22] = {
    -13, 8, -10, 14, -7, 16, -11, 9, -5, 18, -12,
    10, -8, 20, -6, 22, -9, 17, -4, 23, -14, 12,
};

const unsigned axis1_wave18_window_unsigned_offsets[22] = {
    10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u,
    54u, 58u, 62u, 66u, 70u, 74u, 78u, 82u, 86u, 90u, 94u,
};

const unsigned* axis1_wave18_window_windows[25] = {
    axis1_wave18_window_weights + 0, axis1_wave18_window_weights + 2,
    axis1_wave18_window_weights + 4, axis1_wave18_window_weights + 6,
    axis1_wave18_window_weights + 8, axis1_wave18_window_weights + 10,
    axis1_wave18_window_weights + 12, axis1_wave18_window_weights + 14,
    axis1_wave18_window_weights + 16, axis1_wave18_window_weights + 18,
    axis1_wave18_window_weights + 20, axis1_wave18_window_weights + 22,
    axis1_wave18_window_weights + 24, axis1_wave18_window_weights + 26,
    axis1_wave18_window_weights + 28, axis1_wave18_window_weights + 30,
    axis1_wave18_window_weights + 32, axis1_wave18_window_weights + 34,
    axis1_wave18_window_weights + 36, axis1_wave18_window_weights + 38,
    axis1_wave18_window_weights + 40, axis1_wave18_window_weights + 42,
    axis1_wave18_window_weights + 44, axis1_wave18_window_weights + 46,
    axis1_wave18_window_weights + 48,
};

const unsigned** axis1_wave18_window_routes[12] = {
    axis1_wave18_window_windows + 0, axis1_wave18_window_windows + 2,
    axis1_wave18_window_windows + 4, axis1_wave18_window_windows + 6,
    axis1_wave18_window_windows + 8, axis1_wave18_window_windows + 10,
    axis1_wave18_window_windows + 1, axis1_wave18_window_windows + 3,
    axis1_wave18_window_windows + 5, axis1_wave18_window_windows + 7,
    axis1_wave18_window_windows + 9, axis1_wave18_window_windows + 11,
};

const unsigned*** axis1_wave18_window_plans[3] = {
    axis1_wave18_window_routes + 0,
    axis1_wave18_window_routes + 4,
    axis1_wave18_window_routes + 7,
};
