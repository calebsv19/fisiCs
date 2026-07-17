struct bucket10_wave65_node {
    int value;
    int weight;
};

struct bucket10_wave65_node bucket10_wave65_nodes[3];

int bucket10_wave65_struct_libb_step(int step) {
    extern struct bucket10_wave65_node bucket10_wave65_nodes[];

    bucket10_wave65_nodes[1].value += step + bucket10_wave65_nodes[0].value;
    bucket10_wave65_nodes[2].weight += bucket10_wave65_nodes[1].value;
    {
        int external_value = bucket10_wave65_nodes[0].value;
        int bucket10_wave65_nodes[2] = {external_value, step};
        return bucket10_wave65_nodes[0] - bucket10_wave65_nodes[1];
    }
}

int bucket10_wave65_struct_sum(void) {
    extern struct bucket10_wave65_node bucket10_wave65_nodes[];

    return bucket10_wave65_nodes[0].value + bucket10_wave65_nodes[0].weight +
           bucket10_wave65_nodes[1].value + bucket10_wave65_nodes[1].weight +
           bucket10_wave65_nodes[2].value + bucket10_wave65_nodes[2].weight;
}
