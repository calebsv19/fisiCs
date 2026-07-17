#include "15__probe_abi_wave132_units_envelope_callback_handoff.h"
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

static struct Wave132Cell wave132_make_cell(double feet,
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

    struct Wave132Cell cell;
    cell.meters = distance_m;
    cell.joules = reserve_j;
    cell.coulombs = charge_c;
    cell.seconds = window_s;
    cell.payload.named.lane = seed + (unsigned)(distance_m * 256.0);
    cell.payload.named.tag = rotl32(seed ^ (unsigned)reserve_j, seed + 5u);
    cell.payload.named.checksum = seed * 2246822519u + (unsigned)(charge_c * 8.0);
    cell.payload.named.route = (cell.payload.named.lane ^ cell.payload.named.tag) % 3u;
    cell.payload.named.generation = seed ^ (unsigned)(window_s * 100000.0);
    cell.payload.named.owner = rotl32(cell.payload.named.checksum + cell.payload.named.generation, 11u);
    return cell;
}

struct Wave132Envelope wave132_seed_envelope(double feet, double watt_hours, double milliamp_hours, double millis, unsigned seed) {
    struct Wave132Envelope envelope;
    unsigned frame;
    unsigned slot;

    for (frame = 0u; frame < 2u; ++frame) {
        envelope.frames[frame].epoch = seed + frame * 97u;
        for (slot = 0u; slot < 3u; ++slot) {
            unsigned slot_seed = seed + frame * 31u + slot * 13u;
            envelope.frames[frame].cells[slot] =
                wave132_make_cell(feet + (double)(frame * 3u + slot) * 0.625,
                                  watt_hours + (double)(frame + slot + 1u) * 0.21875,
                                  milliamp_hours + (double)(frame * 53u + slot * 29u),
                                  millis + (double)(frame * 11u + slot * 7u),
                                  slot_seed);
            envelope.frames[frame].epoch ^= envelope.frames[frame].cells[slot].payload.named.owner;
        }
    }

    envelope.footer = envelope.frames[0].cells[1].payload;
    envelope.footer.named.owner ^= envelope.frames[1].cells[2].payload.named.checksum;
    envelope.generation = seed ^ envelope.frames[0].epoch ^ rotl32(envelope.frames[1].epoch, 7u);
    return envelope;
}

struct Wave132Frame wave132_bias_frame(struct Wave132Frame frame, struct Wave132Cell carry, unsigned step) {
    unsigned slot = (frame.epoch + carry.payload.named.route + step) % 3u;
    struct Wave132Cell next = frame.cells[slot];

    next.meters += carry.meters * 0.125 + (double)((step & 7u) + 1u) * 0.03125;
    next.joules += carry.joules * 0.0625 + next.meters * (double)(step + 2u);
    next.coulombs += carry.coulombs * 0.25 + (double)(slot + 1u) * 0.125;
    next.seconds += carry.seconds * 0.5 + (double)(step & 3u) * 0.0005;
    next.payload.words[slot] ^= rotl32(carry.payload.named.owner + step * 41u, slot + 6u);
    next.payload.named.generation += carry.payload.named.generation ^ step;
    next.payload.named.owner = rotl32(next.payload.named.owner ^ next.payload.named.generation, step + 9u);

    frame.cells[slot] = next;
    frame.epoch = rotl32(frame.epoch ^ next.payload.named.owner ^ carry.payload.named.checksum, (step & 7u) + 4u);
    return frame;
}

struct Wave132Frame wave132_shear_frame(struct Wave132Frame frame, struct Wave132Cell carry, unsigned step) {
    unsigned first = (carry.payload.named.route + step) % 3u;
    unsigned second = (first + 2u) % 3u;
    frame.cells[first].joules += carry.meters * carry.seconds * (double)(step + 1u);
    frame.cells[first].payload.named.tag ^= rotl32(carry.payload.named.lane + step * 67u, first + 8u);
    frame.cells[second].meters += frame.cells[first].coulombs * 0.015625;
    frame.cells[second].payload.named.checksum += frame.cells[first].payload.named.tag ^ carry.payload.named.owner;
    return wave132_bias_frame(frame, frame.cells[first], step + 5u);
}

struct Wave132Envelope wave132_apply_envelope(struct Wave132Envelope envelope, Wave132FrameFn callback, unsigned step) {
    unsigned src = (envelope.generation + step) & 1u;
    unsigned dst = src ^ 1u;
    unsigned carry_slot = (envelope.frames[src].epoch + step) % 3u;
    struct Wave132Cell carry = envelope.frames[src].cells[carry_slot];
    struct Wave132Frame next = callback(envelope.frames[dst], carry, step);

    envelope.frames[dst] = wave132_bias_frame(next, envelope.frames[src].cells[(carry_slot + 1u) % 3u], step + 3u);
    envelope.frames[src] = wave132_shear_frame(envelope.frames[src], envelope.frames[dst].cells[(carry_slot + 2u) % 3u], step + 1u);
    envelope.footer.words[step % 6u] ^= envelope.frames[dst].epoch + rotl32(envelope.frames[src].epoch, step + 2u);
    envelope.generation = rotl32(envelope.generation ^ envelope.footer.named.owner ^ envelope.frames[dst].epoch, (step & 7u) + 5u);
    return envelope;
}

unsigned wave132_envelope_digest(struct Wave132Envelope envelope, unsigned salt) {
    unsigned acc = salt ^ envelope.generation ^ envelope.footer.named.checksum;
    unsigned frame;
    unsigned slot;

    for (frame = 0u; frame < 2u; ++frame) {
        acc ^= rotl32(envelope.frames[frame].epoch, frame + 3u);
        for (slot = 0u; slot < 3u; ++slot) {
            struct Wave132Cell cell = envelope.frames[frame].cells[slot];
            unsigned dm = (unsigned)(cell.meters * 1000.0);
            unsigned rj = (unsigned)(cell.joules);
            unsigned cc = (unsigned)(cell.coulombs * 32.0);
            unsigned ts = (unsigned)(cell.seconds * 1000000.0);
            acc ^= dm + rotl32(rj, slot + 5u) + rotl32(cc, frame + slot + 7u);
            acc = rotl32(acc + ts + cell.payload.words[(frame + slot) % 6u], slot * 4u + frame + 9u);
        }
    }

    return acc ^ envelope.footer.words[5] ^ rotl32(envelope.footer.words[2], 13u);
}
