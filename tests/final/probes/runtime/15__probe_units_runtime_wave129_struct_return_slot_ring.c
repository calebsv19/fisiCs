#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "meter_per_second") == 0) return value * 0.3048;
    if (strcmp(target_unit, "second") == 0) return value / 1000.0;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "ampere_hour") == 0) return value / 1000.0;
    return value;
}
#endif

struct Wave129Sample {
    double distance_m;
    double reserve_j;
    double charge_ah;
};

struct Wave129Slot {
    struct Wave129Sample sample;
    double fold[2];
};

static struct Wave129Sample make_sample(double feet, double tick_ms,
                                        double reserve_wh, double draw_mah,
                                        double speed_fps) {
    double raw_distance
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = feet;
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(raw_distance, "meter");

    double tick
        [[fisics::dim(time)]]
        [[fisics::unit(millisecond)]] = tick_ms;
    double tick_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(tick, "second");

    double speed
        [[fisics::dim(speed)]]
        [[fisics::unit(foot_per_second)]] = speed_fps;
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] = fisics_convert_unit(speed, "meter_per_second");

    double reserve
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = reserve_wh;
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve, "joule");

    double draw
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = draw_mah;
    double charge_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = fisics_convert_unit(draw, "ampere_hour");

    struct Wave129Sample sample;
    sample.distance_m = distance_m + speed_mps * tick_s;
    sample.reserve_j = reserve_j;
    sample.charge_ah = charge_ah;
    return sample;
}

static struct Wave129Slot make_slot(struct Wave129Sample sample, int lane) {
    struct Wave129Slot slot;
    slot.sample = sample;
    slot.fold[0] = sample.distance_m * (double)(lane + 3) + sample.charge_ah * 11.0;
    slot.fold[1] = sample.reserve_j / (double)(lane + 5) + sample.distance_m * 7.0;
    return slot;
}

int main(void) {
    struct Wave129Slot ring[3];
    double checksum = 0.0;
    int i;

    ring[0] = make_slot(make_sample(4.5, 360.0, 0.75, 180.0, 12.0), 0);
    ring[1] = make_slot(make_sample(8.0, 540.0, 1.25, 240.0, 9.5), 1);
    ring[2] = make_slot(make_sample(2.75, 720.0, 0.5, 150.0, 15.25), 2);

    ring[1].sample = ring[0].sample;
    ring[2].fold[0] += ring[1].sample.distance_m * 2.0;
    ring[0].fold[1] += ring[2].sample.charge_ah * 13.0;

    for (i = 0; i < 3; ++i) {
        checksum += ring[i].fold[0] * (double)(i + 2);
        checksum += ring[(i + 1) % 3].fold[1] / (double)(i + 3);
        checksum += ring[i].sample.reserve_j / (double)(700 + i * 17);
    }

    printf("%.6f %.6f %.6f\n", ring[2].fold[0], ring[0].fold[1], checksum);
    return 0;
}
