#include "15__probe_abi_wave131_units_nested_aggregate_callback_matrix.h"
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "coulomb") == 0) return value * 3.6;
    return value;
}
#endif

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

static struct Wave131Sample wave131_make_sample(double feet,
                                                double watt_hours,
                                                double milliamp_hours,
                                                unsigned seed) {
    double distance_ft
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = feet;
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(distance_ft, "meter");

    double reserve_wh
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = watt_hours;
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");

    double charge_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = milliamp_hours;
    double charge_c
        [[fisics::dim(charge)]]
        [[fisics::unit(coulomb)]] = fisics_convert_unit(charge_mah, "coulomb");

    struct Wave131Sample sample;
    sample.meters = distance_m;
    sample.joules = reserve_j;
    sample.coulombs = charge_c;
    sample.payload.named.lane = seed + (unsigned)(distance_m * 128.0);
    sample.payload.named.tag = rotl32(seed ^ (unsigned)reserve_j, seed + 3u);
    sample.payload.named.mix = seed * 2654435761u + (unsigned)(charge_c * 4.0);
    sample.payload.named.route = (sample.payload.named.lane ^ sample.payload.named.tag ^ sample.payload.named.mix) & 3u;
    return sample;
}

struct Wave131Packet wave131_seed_packet(double feet, double watt_hours, double milliamp_hours, unsigned seed) {
    struct Wave131Packet packet;
    unsigned row;
    unsigned col;

    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            unsigned slot_seed = seed + row * 19u + col * 7u;
            packet.samples[row][col] = wave131_make_sample(feet + (double)(row * 2u + col) * 0.875,
                                                           watt_hours + (double)(row + col + 1u) * 0.1875,
                                                           milliamp_hours + (double)(row * 41u + col * 23u),
                                                           slot_seed);
        }
    }

    packet.epoch = seed ^ packet.samples[0][0].payload.named.mix ^ rotl32(packet.samples[1][1].payload.named.tag, 9u);
    return packet;
}

struct Wave131Sample wave131_bias_sample(struct Wave131Sample sample, unsigned step) {
    unsigned route = (sample.payload.named.route + step) & 3u;
    sample.meters += (double)((step & 5u) + 1u) * 0.0625;
    sample.joules += sample.meters * (double)(step + 2u);
    sample.coulombs += (double)(route + 1u) * 0.5;
    sample.payload.words[route] ^= rotl32(sample.payload.named.mix + step * 17u, route + 5u);
    sample.payload.named.route = (sample.payload.named.route + route + (sample.payload.words[route] & 3u)) & 3u;
    return sample;
}

struct Wave131Sample wave131_cross_sample(struct Wave131Sample sample, unsigned step) {
    sample.joules += sample.coulombs * 0.375 + sample.meters * 2.0;
    sample.payload.named.tag ^= rotl32(sample.payload.named.lane + step * 29u, step + 7u);
    sample.payload.named.mix += sample.payload.named.tag ^ rotl32(sample.payload.named.route + step, 13u);
    return wave131_bias_sample(sample, step + 3u);
}

struct Wave131Packet wave131_apply_packet(struct Wave131Packet packet, Wave131SampleFn callback, unsigned step) {
    unsigned src_row = (packet.epoch + step) & 1u;
    unsigned src_col = (packet.samples[src_row][0].payload.named.route + step) & 1u;
    unsigned dst_row = src_row ^ 1u;
    unsigned dst_col = src_col ^ 1u;
    struct Wave131Sample next = callback(packet.samples[src_row][src_col], step);

    packet.samples[dst_row][dst_col] = wave131_bias_sample(next, step + packet.samples[src_row][src_col].payload.named.route + 1u);
    packet.samples[src_row][dst_col] = wave131_cross_sample(packet.samples[src_row][dst_col], step + 2u);
    packet.epoch = rotl32(packet.epoch ^ packet.samples[dst_row][dst_col].payload.named.mix, (step & 7u) + 6u);
    return packet;
}

unsigned wave131_packet_digest(struct Wave131Packet packet, unsigned salt) {
    unsigned acc = salt ^ packet.epoch;
    unsigned row;
    unsigned col;

    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            struct Wave131Sample sample = packet.samples[row][col];
            unsigned dm = (unsigned)(sample.meters * 1000.0);
            unsigned rj = (unsigned)(sample.joules);
            unsigned cc = (unsigned)(sample.coulombs * 16.0);
            unsigned slot = (row << 1u) | col;
            acc ^= dm + rotl32(rj, slot + 3u) + sample.payload.words[slot];
            acc = rotl32(acc + cc + sample.payload.named.route, slot * 5u + 7u);
        }
    }

    return acc ^ packet.samples[0][1].payload.words[3] ^ rotl32(packet.samples[1][0].payload.words[2], 11u);
}
