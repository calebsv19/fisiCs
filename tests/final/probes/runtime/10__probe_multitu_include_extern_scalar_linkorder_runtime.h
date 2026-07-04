extern int bucket10_linkorder_extern_scalar;

void bucket10_linkorder_extern_reset_lane(int lane_seed);
void bucket10_linkorder_extern_reset_bias(int bias_seed);
int bucket10_linkorder_extern_seed(int seed);
int bucket10_linkorder_extern_add(int delta);
int bucket10_linkorder_extern_mix(int seed);
int bucket10_linkorder_extern_peek(void);
