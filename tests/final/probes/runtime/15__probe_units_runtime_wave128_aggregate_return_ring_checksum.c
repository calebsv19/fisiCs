#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "second") == 0) return value / 1000.0;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "bar") == 0) return value * 0.06894757293168361;
    return value;
}
#endif

struct UnitSample {
    double distance_m;
    double reserve_j;
    double pressure_bar;
};

struct UnitPacket {
    struct UnitSample samples[3];
    double fold[2];
};

static struct UnitPacket make_packet(const double feet[3],
                                     const double wh[3],
                                     const double psi[3],
                                     double window_ms) {
    struct UnitPacket packet = {{{0.0, 0.0, 0.0}}, {0.0, 0.0}};
    double window
        [[fisics::dim(time)]]
        [[fisics::unit(millisecond)]] = window_ms;
    double window_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(window, "second");
    int i;

    for (i = 0; i < 3; ++i) {
        double raw_distance
            [[fisics::dim(length)]]
            [[fisics::unit(foot)]] = feet[i];
        double distance_m
            [[fisics::dim(length)]]
            [[fisics::unit(meter)]] = fisics_convert_unit(raw_distance, "meter");

        double reserve
            [[fisics::dim(energy)]]
            [[fisics::unit(watt_hour)]] = wh[i];
        double reserve_j
            [[fisics::dim(energy)]]
            [[fisics::unit(joule)]] = fisics_convert_unit(reserve, "joule");

        double pressure
            [[fisics::dim(pressure)]]
            [[fisics::unit(psi)]] = psi[i];
        double pressure_bar
            [[fisics::dim(pressure)]]
            [[fisics::unit(bar)]] = fisics_convert_unit(pressure, "bar");

        packet.samples[i].distance_m = distance_m + window_s * (double)(i + 1);
        packet.samples[i].reserve_j = reserve_j;
        packet.samples[i].pressure_bar = pressure_bar;
        packet.fold[0] += packet.samples[i].distance_m * (double)(i + 2);
        packet.fold[1] += packet.samples[i].reserve_j / 500.0 +
                          packet.samples[i].pressure_bar * (double)(i + 4);
    }

    return packet;
}

static double packet_score(struct UnitPacket packet, int salt) {
    double score = packet.fold[0] * 1.25 + packet.fold[1] * 0.75;
    int i;

    for (i = 0; i < 3; ++i) {
        score += packet.samples[(i + salt) % 3].distance_m * (double)(salt + i + 3);
        score += packet.samples[i].reserve_j / (double)(900 + salt * 11 + i);
        score += packet.samples[i].pressure_bar * (double)(i + 5);
    }

    return score;
}

int main(void) {
    double feet_a[3] = {4.5, 2.75, 8.25};
    double wh_a[3] = {0.5, 1.25, 0.875};
    double psi_a[3] = {14.0, 18.5, 11.25};
    double feet_b[3] = {6.0, 3.5, 9.75};
    double wh_b[3] = {1.125, 0.625, 1.75};
    double psi_b[3] = {20.0, 12.0, 16.25};
    struct UnitPacket ring[2];
    double checksum;

    ring[0] = make_packet(feet_a, wh_a, psi_a, 375.0);
    ring[1] = make_packet(feet_b, wh_b, psi_b, 625.0);

    ring[1].samples[1] = ring[0].samples[2];
    ring[0].fold[0] += ring[1].samples[0].distance_m;
    ring[1].fold[1] += ring[0].samples[1].pressure_bar * 7.0;

    checksum = packet_score(ring[0], 1) + packet_score(ring[1], 2);
    printf("%.6f %.6f %.6f\n",
           ring[0].fold[0],
           ring[1].fold[1],
           checksum);
    return 0;
}
