const unsigned axis1_wave17_alias_weights[44] = {
    10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u,
    54u, 58u, 62u, 66u, 70u, 74u, 78u, 82u, 86u, 90u, 94u,
    98u, 102u, 106u, 110u, 114u, 118u, 122u, 126u, 130u, 134u,
    138u, 142u, 146u, 150u, 154u, 158u, 162u, 166u, 170u, 174u,
    178u, 182u,
};

const int axis1_wave17_alias_signed_offsets[22] = {
    -13, 6, -10, 12, -7, 14, -11, 8, -5, 16, -12,
    9, -8, 18, -6, 20, -9, 15, -4, 21, -14, 11,
};

const unsigned axis1_wave17_alias_unsigned_offsets[22] = {
    7u, 11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u,
    51u, 55u, 59u, 63u, 67u, 71u, 75u, 79u, 83u, 87u, 91u,
};

const unsigned* axis1_wave17_alias_windows[22] = {
    axis1_wave17_alias_weights + 0,
    axis1_wave17_alias_weights + 2,
    axis1_wave17_alias_weights + 4,
    axis1_wave17_alias_weights + 6,
    axis1_wave17_alias_weights + 8,
    axis1_wave17_alias_weights + 10,
    axis1_wave17_alias_weights + 12,
    axis1_wave17_alias_weights + 14,
    axis1_wave17_alias_weights + 16,
    axis1_wave17_alias_weights + 18,
    axis1_wave17_alias_weights + 20,
    axis1_wave17_alias_weights + 22,
    axis1_wave17_alias_weights + 24,
    axis1_wave17_alias_weights + 26,
    axis1_wave17_alias_weights + 28,
    axis1_wave17_alias_weights + 30,
    axis1_wave17_alias_weights + 32,
    axis1_wave17_alias_weights + 34,
    axis1_wave17_alias_weights + 36,
    axis1_wave17_alias_weights + 38,
    axis1_wave17_alias_weights + 40,
    axis1_wave17_alias_weights + 42,
};

const unsigned** axis1_wave17_alias_routes[11] = {
    axis1_wave17_alias_windows + 0,
    axis1_wave17_alias_windows + 2,
    axis1_wave17_alias_windows + 4,
    axis1_wave17_alias_windows + 6,
    axis1_wave17_alias_windows + 8,
    axis1_wave17_alias_windows + 10,
    axis1_wave17_alias_windows + 1,
    axis1_wave17_alias_windows + 3,
    axis1_wave17_alias_windows + 5,
    axis1_wave17_alias_windows + 7,
    axis1_wave17_alias_windows + 9,
};

const unsigned*** axis1_wave17_alias_plans[4] = {
    axis1_wave17_alias_routes + 0,
    axis1_wave17_alias_routes + 2,
    axis1_wave17_alias_routes + 4,
    axis1_wave17_alias_routes + 6,
};
