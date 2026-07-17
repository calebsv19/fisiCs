#include "15__probe_abi_wave130_units_callback_aggregate_preservation.h"
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

static struct Wave130Cell wave130_make_cell(double feet,
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

    double draw_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = milliamp_hours;
    double charge_c
        [[fisics::dim(charge)]]
        [[fisics::unit(coulomb)]] = fisics_convert_unit(draw_mah, "coulomb");

    struct Wave130Cell cell;
    cell.distance_m = distance_m;
    cell.reserve_j = reserve_j;
    cell.charge_c = charge_c;
    cell.bits.named.lo = seed * 33u + (unsigned)(distance_m * 100.0);
    cell.bits.named.hi = rotl32(seed ^ (unsigned)reserve_j, seed + 5u);
    cell.bits.named.tag = seed * 1103515245u + (unsigned)charge_c;
    cell.bits.named.route = (cell.bits.named.lo ^ cell.bits.named.hi ^ cell.bits.named.tag) & 3u;
    return cell;
}

struct Wave130Frame wave130_seed_frame(double feet, double watt_hours, double milliamp_hours, unsigned seed) {
    struct Wave130Frame frame;
    frame.cells[0] = wave130_make_cell(feet, watt_hours, milliamp_hours, seed);
    frame.cells[1] = wave130_make_cell(feet * 0.5 + 1.25,
                                       watt_hours * 1.5 + 0.125,
                                       milliamp_hours + 37.5,
                                       seed + 17u);
    frame.epoch = seed ^ frame.cells[0].bits.named.tag ^ rotl32(frame.cells[1].bits.named.hi, 7u);
    return frame;
}

struct Wave130Cell wave130_adjust_cell(struct Wave130Cell cell, unsigned step) {
    unsigned slot = (cell.bits.named.route + step) & 3u;
    cell.distance_m += (double)(step & 7u) * 0.03125 + cell.charge_c * 0.00025;
    cell.reserve_j += cell.distance_m * (double)(step + 3u);
    cell.charge_c += (double)(slot + 1u) * 0.75;
    cell.bits.words[slot] ^= rotl32(cell.bits.named.lo + cell.bits.named.tag + step, slot + 3u);
    cell.bits.named.route = (cell.bits.named.route + step + (cell.bits.words[slot] & 3u)) & 3u;
    return cell;
}

struct Wave130Cell wave130_fold_callback(struct Wave130Cell cell, unsigned step) {
    cell.reserve_j += cell.distance_m * 0.5 + (double)(step * 11u);
    cell.bits.named.hi ^= rotl32(cell.bits.named.route + step * 19u, step + 9u);
    cell.bits.named.tag += cell.bits.named.lo ^ rotl32(cell.bits.named.hi, step + 1u);
    return wave130_adjust_cell(cell, step + 5u);
}

struct Wave130Frame wave130_apply_frame(struct Wave130Frame frame, Wave130CellFn callback, unsigned step) {
    unsigned src = (frame.epoch + step) & 1u;
    unsigned dst = src ^ 1u;
    struct Wave130Cell next = callback(frame.cells[src], step);
    frame.cells[dst] = wave130_adjust_cell(next, step + frame.cells[src].bits.named.route + 1u);
    frame.epoch = rotl32(frame.epoch ^ frame.cells[dst].bits.named.tag, (step & 7u) + 5u);
    return frame;
}

unsigned wave130_frame_digest(struct Wave130Frame frame, unsigned salt) {
    unsigned acc = salt ^ frame.epoch;
    unsigned i;
    for (i = 0u; i < 2u; ++i) {
        unsigned dm = (unsigned)(frame.cells[i].distance_m * 1000.0);
        unsigned rj = (unsigned)(frame.cells[i].reserve_j);
        unsigned cc = (unsigned)(frame.cells[i].charge_c * 10.0);
        acc ^= dm + rotl32(rj, i + 3u) + frame.cells[i].bits.words[i];
        acc = rotl32(acc ^ cc ^ frame.cells[i].bits.named.route, i * 9u + 7u);
    }
    return acc ^ frame.cells[0].bits.words[3] ^ rotl32(frame.cells[1].bits.words[2], 11u);
}
