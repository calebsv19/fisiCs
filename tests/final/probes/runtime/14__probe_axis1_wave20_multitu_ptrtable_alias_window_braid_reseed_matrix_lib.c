const unsigned axis1_wave20_alias_weights[58] = {
    17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u,
    65u, 69u, 73u, 77u, 81u, 85u, 89u, 93u, 97u, 101u, 105u, 109u,
    113u, 117u, 121u, 125u, 129u, 133u, 137u, 141u, 145u, 149u, 153u, 157u,
    161u, 165u, 169u, 173u, 177u, 181u, 185u, 189u, 193u, 197u, 201u, 205u,
    209u, 213u, 217u, 221u, 225u, 229u, 233u, 237u, 241u, 245u,
};

const unsigned axis1_wave20_alias_lane_masks[14] = {
    3u, 6u, 9u, 5u, 10u, 12u, 7u, 11u, 13u, 14u, 8u, 15u, 17u, 19u,
};

const int axis1_wave20_alias_signed_offsets[26] = {
    -15, 10, -12, 16, -9, 18, -13, 11, -7, 20, -14, 12, -10,
    22, -8, 24, -11, 19, -6, 25, -16, 14, -5, 26, -4, 28,
};

const unsigned axis1_wave20_alias_unsigned_offsets[26] = {
    14u, 18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u,
    66u, 70u, 74u, 78u, 82u, 86u, 90u, 94u, 98u, 102u, 106u, 110u, 114u,
};

const unsigned* axis1_wave20_alias_windows[29] = {
    axis1_wave20_alias_weights + 0, axis1_wave20_alias_weights + 2,
    axis1_wave20_alias_weights + 4, axis1_wave20_alias_weights + 6,
    axis1_wave20_alias_weights + 8, axis1_wave20_alias_weights + 10,
    axis1_wave20_alias_weights + 12, axis1_wave20_alias_weights + 14,
    axis1_wave20_alias_weights + 16, axis1_wave20_alias_weights + 18,
    axis1_wave20_alias_weights + 20, axis1_wave20_alias_weights + 22,
    axis1_wave20_alias_weights + 24, axis1_wave20_alias_weights + 26,
    axis1_wave20_alias_weights + 28, axis1_wave20_alias_weights + 30,
    axis1_wave20_alias_weights + 32, axis1_wave20_alias_weights + 34,
    axis1_wave20_alias_weights + 36, axis1_wave20_alias_weights + 38,
    axis1_wave20_alias_weights + 40, axis1_wave20_alias_weights + 42,
    axis1_wave20_alias_weights + 44, axis1_wave20_alias_weights + 46,
    axis1_wave20_alias_weights + 48, axis1_wave20_alias_weights + 50,
    axis1_wave20_alias_weights + 52, axis1_wave20_alias_weights + 54,
    axis1_wave20_alias_weights + 56,
};

const unsigned** axis1_wave20_alias_routes[14] = {
    axis1_wave20_alias_windows + 0, axis1_wave20_alias_windows + 2,
    axis1_wave20_alias_windows + 4, axis1_wave20_alias_windows + 6,
    axis1_wave20_alias_windows + 8, axis1_wave20_alias_windows + 10,
    axis1_wave20_alias_windows + 12, axis1_wave20_alias_windows + 1,
    axis1_wave20_alias_windows + 3, axis1_wave20_alias_windows + 5,
    axis1_wave20_alias_windows + 7, axis1_wave20_alias_windows + 9,
    axis1_wave20_alias_windows + 11, axis1_wave20_alias_windows + 13,
};

const unsigned*** axis1_wave20_alias_plans[3] = {
    axis1_wave20_alias_routes + 0,
    axis1_wave20_alias_routes + 5,
    axis1_wave20_alias_routes + 9,
};
