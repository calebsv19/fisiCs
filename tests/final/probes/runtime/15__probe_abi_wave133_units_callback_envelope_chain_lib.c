#include "15__probe_abi_wave133_units_callback_envelope_chain.h"
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "coulomb") == 0) return value * 3.6;
    if (strcmp(target_unit, "second") == 0) return value * 0.001;
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

static struct Wave133ChainCell wave133_chain_make_cell(double feet,
                                                       double watt_hours,
                                                       double milliamp_hours,
                                                       double millis,
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

    double window_ms
        [[fisics::dim(time)]]
        [[fisics::unit(millisecond)]] = millis;
    double window_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(window_ms, "second");

    struct Wave133ChainCell cell;
    cell.meters = distance_m;
    cell.joules = reserve_j;
    cell.coulombs = charge_c;
    cell.seconds = window_s;
    cell.payload.named.lane = seed + (unsigned)(distance_m * 512.0);
    cell.payload.named.route = (seed ^ (unsigned)reserve_j ^ (unsigned)(charge_c * 4.0)) & 3u;
    cell.payload.named.tag = rotl32(seed + (unsigned)(charge_c * 32.0), seed + 9u);
    cell.payload.named.mix = seed * 2246822519u + (unsigned)(window_s * 1000000.0);
    cell.payload.named.generation = seed ^ (unsigned)(reserve_j) ^ rotl32(cell.payload.named.tag, 7u);
    cell.payload.named.owner = rotl32(cell.payload.named.lane ^ cell.payload.named.mix ^ cell.payload.named.generation, 13u);
    return cell;
}

static struct Wave133ChainSegment wave133_chain_seed_segment(double feet,
                                                            double watt_hours,
                                                            double milliamp_hours,
                                                            double millis,
                                                            unsigned seed) {
    struct Wave133ChainSegment segment;
    unsigned slot;

    segment.epoch = seed ^ 0x7f4a7c15u;
    for (slot = 0u; slot < 2u; ++slot) {
        unsigned cell_seed = seed + slot * 41u;
        segment.cells[slot] =
            wave133_chain_make_cell(feet + (double)slot * 0.875,
                                    watt_hours + (double)(slot + 1u) * 0.1875,
                                    milliamp_hours + (double)(slot * 43u),
                                    millis + (double)(slot * 13u),
                                    cell_seed);
        segment.epoch ^= segment.cells[slot].payload.named.owner;
    }
    segment.segment_footer = segment.cells[1].payload;
    segment.segment_footer.words[1] ^= segment.cells[0].payload.named.mix;
    return segment;
}

struct Wave133ChainEnvelope wave133_chain_seed(double feet, double watt_hours, double milliamp_hours, double millis, unsigned seed) {
    struct Wave133ChainEnvelope envelope;
    unsigned row;
    unsigned col;

    envelope.generation = seed ^ 0x6a09e667u;
    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            unsigned segment_seed = seed + row * 53u + col * 29u;
            envelope.segments[row][col] =
                wave133_chain_seed_segment(feet + (double)(row * 2u + col) * 0.5,
                                           watt_hours + (double)(row + col) * 0.25,
                                           milliamp_hours + (double)(row * 67u + col * 31u),
                                           millis + (double)(row * 19u + col * 7u),
                                           segment_seed);
            envelope.generation ^= envelope.segments[row][col].epoch;
        }
    }
    envelope.envelope_footer = envelope.segments[0][1].segment_footer;
    envelope.envelope_footer.words[4] ^= envelope.segments[1][0].segment_footer.named.owner;
    return envelope;
}

struct Wave133ChainSegment wave133_chain_bias_segment(struct Wave133ChainSegment segment, struct Wave133ChainCell carry, unsigned step) {
    unsigned slot = (segment.epoch + carry.payload.named.route + step) & 1u;
    struct Wave133ChainCell next = segment.cells[slot];

    next.meters += carry.meters * 0.1875 + (double)((step & 7u) + 1u) * 0.03125;
    next.joules += carry.joules * 0.09375 + next.meters * (double)(step + 2u);
    next.coulombs += carry.coulombs * 0.375 + (double)(slot + 1u) * 0.125;
    next.seconds += carry.seconds * 0.5 + (double)(step & 3u) * 0.0005;
    next.payload.words[(step + slot) % 6u] ^= rotl32(carry.payload.named.owner + segment.epoch, slot + step + 4u);
    next.payload.named.generation += carry.payload.named.generation ^ step;
    next.payload.named.owner = rotl32(next.payload.named.owner ^ next.payload.named.generation, step + 11u);

    segment.cells[slot] = next;
    segment.segment_footer.words[(slot + step) % 6u] ^= next.payload.named.owner + carry.payload.named.tag;
    segment.epoch = rotl32(segment.epoch ^ next.payload.named.owner ^ segment.segment_footer.named.mix, (step & 7u) + 5u);
    return segment;
}

struct Wave133ChainSegment wave133_chain_cross_segment(struct Wave133ChainSegment segment, struct Wave133ChainCell carry, unsigned step) {
    unsigned first = (carry.payload.named.route + step) & 1u;
    unsigned second = first ^ 1u;
    segment.cells[first].joules += carry.meters * carry.seconds * (double)(step + 3u);
    segment.cells[first].payload.named.tag ^= rotl32(carry.payload.named.lane + step * 97u, first + 9u);
    segment.cells[second].meters += segment.cells[first].coulombs * 0.015625;
    segment.cells[second].payload.named.mix += segment.cells[first].payload.named.tag ^ carry.payload.named.owner;
    return wave133_chain_bias_segment(segment, segment.cells[first], step + 6u);
}

struct Wave133ChainEnvelope wave133_chain_apply(struct Wave133ChainEnvelope envelope, Wave133SegmentFn callback, unsigned step) {
    unsigned src_row = (envelope.generation + step) & 1u;
    unsigned src_col = (envelope.envelope_footer.named.route + step) & 1u;
    unsigned dst_row = src_row ^ 1u;
    unsigned dst_col = src_col ^ 1u;
    unsigned carry_slot = (envelope.segments[src_row][src_col].epoch + step) & 1u;
    struct Wave133ChainCell carry = envelope.segments[src_row][src_col].cells[carry_slot];
    struct Wave133ChainSegment next = callback(envelope.segments[dst_row][dst_col], carry, step);

    envelope.segments[dst_row][dst_col] =
        wave133_chain_bias_segment(next, envelope.segments[src_row][src_col].cells[carry_slot ^ 1u], step + 4u);
    envelope.segments[src_row][src_col] =
        wave133_chain_cross_segment(envelope.segments[src_row][src_col], envelope.segments[dst_row][dst_col].cells[step & 1u], step + 2u);
    envelope.envelope_footer.words[step % 6u] ^= envelope.segments[dst_row][dst_col].epoch + rotl32(envelope.segments[src_row][src_col].epoch, step + 3u);
    envelope.generation = rotl32(envelope.generation ^ envelope.envelope_footer.named.owner ^ envelope.segments[dst_row][dst_col].epoch, (step & 7u) + 6u);
    return envelope;
}

unsigned wave133_chain_digest(struct Wave133ChainEnvelope envelope, unsigned salt) {
    unsigned acc = salt ^ envelope.generation ^ envelope.envelope_footer.named.owner;
    unsigned row;
    unsigned col;
    unsigned slot;

    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            struct Wave133ChainSegment segment = envelope.segments[row][col];
            acc ^= rotl32(segment.epoch, row * 5u + col + 3u);
            for (slot = 0u; slot < 2u; ++slot) {
                struct Wave133ChainCell cell = segment.cells[slot];
                unsigned dm = (unsigned)(cell.meters * 1000.0);
                unsigned rj = (unsigned)(cell.joules);
                unsigned cc = (unsigned)(cell.coulombs * 32.0);
                unsigned ts = (unsigned)(cell.seconds * 1000000.0);
                acc ^= dm + rotl32(rj, slot + 6u) + rotl32(cc, row + col + slot + 8u);
                acc = rotl32(acc + ts + cell.payload.words[(row + col + slot) % 6u], row * 7u + col * 3u + slot + 5u);
            }
            acc ^= segment.segment_footer.words[(row + col) % 6u];
        }
    }

    return acc ^ envelope.envelope_footer.words[5] ^ rotl32(envelope.envelope_footer.words[2], 15u);
}
