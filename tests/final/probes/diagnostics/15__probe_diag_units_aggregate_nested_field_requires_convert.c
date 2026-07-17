struct Sample {
    double distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]];
};

struct Packet {
    struct Sample sample;
};

int main(void) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = 6.0;
    struct Packet packet = { { distance_ft } };
    return (int)packet.sample.distance;
}
