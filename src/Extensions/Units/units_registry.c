// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_registry.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

/*
 * Step-1 registry policy lock:
 *
 * - Canonical names are the only exported names. Aliases exist only for source
 *   acceptance and round-trip diagnostics.
 * - Family grouping is semantic, not merely textual. Every entry in a family
 *   must share that family's exact canonical `Dim8`.
 * - Canonical-family units use `scale_to_canonical = 1.0`.
 * - Noncanonical units convert through the family canonical base with a
 *   positive, nonzero scale.
 * - Offset support is intentionally dormant for now; nonzero offsets should be
 *   treated as a future dedicated expansion lane rather than added casually.
 */
static const char* const kMeterAliases[] = {"meters", "metre", "metres"};
static const char* const kMillimeterAliases[] = {"millimeters", "millimetre", "millimetres"};
static const char* const kCentimeterAliases[] = {"centimeters", "centimetre", "centimetres"};
static const char* const kKilometerAliases[] = {"kilometers", "kilometre", "kilometres"};
static const char* const kFootAliases[] = {"feet"};
static const char* const kInchAliases[] = {"inches"};
static const char* const kYardAliases[] = {"yards"};
static const char* const kMileAliases[] = {"miles"};
static const char* const kKilogramAliases[] = {"kilograms"};
static const char* const kGramAliases[] = {"grams"};
static const char* const kMilligramAliases[] = {"milligrams"};
static const char* const kMetricTonAliases[] = {"metric_tons", "tonne", "tonnes"};
static const char* const kPoundAliases[] = {"pounds"};
static const char* const kSecondAliases[] = {"seconds"};
static const char* const kMillisecondAliases[] = {"milliseconds"};
static const char* const kMinuteAliases[] = {"minutes"};
static const char* const kHourAliases[] = {"hours"};
static const char* const kAmpereAliases[] = {"amp", "amps", "amperes"};
static const char* const kKelvinAliases[] = {"kelvins"};
static const char* const kMoleAliases[] = {"moles"};
static const char* const kCandelaAliases[] = {"candelas"};
static const char* const kMeterPerSecondAliases[] = {"meters_per_second"};
static const char* const kFootPerSecondAliases[] = {"feet_per_second"};
static const char* const kMeterPerSecondSquaredAliases[] = {"meters_per_second_squared"};
static const char* const kNewtonAliases[] = {"newtons"};
static const char* const kKilonewtonAliases[] = {"kilonewtons"};
static const char* const kPoundForceAliases[] = {"pound_forces"};
static const char* const kJouleAliases[] = {"joules"};
static const char* const kKilojouleAliases[] = {"kilojoules"};
static const char* const kWattHourAliases[] = {"watt_hours"};
static const char* const kKilowattHourAliases[] = {"kilowatt_hours"};
static const char* const kWattAliases[] = {"watts"};
static const char* const kMilliwattAliases[] = {"milliwatts"};
static const char* const kKilowattAliases[] = {"kilowatts"};
static const char* const kMegawattAliases[] = {"megawatts"};
static const char* const kPascalAliases[] = {"pascals"};
static const char* const kKilopascalAliases[] = {"kilopascals"};
static const char* const kBarAliases[] = {"bars"};
static const char* const kPsiAliases[] = {"psis"};
static const char* const kCoulombAliases[] = {"coulombs"};
static const char* const kAmpereHourAliases[] = {"ampere_hours"};
static const char* const kMilliampereHourAliases[] = {"milliampere_hours"};
static const char* const kVoltAliases[] = {"volts"};
static const char* const kMillivoltAliases[] = {"millivolts"};
static const char* const kKilovoltAliases[] = {"kilovolts"};
static const char* const kOhmAliases[] = {"ohms"};
static const char* const kMilliohmAliases[] = {"milliohms"};
static const char* const kKiloohmAliases[] = {"kiloohms"};
static const char* const kMegaohmAliases[] = {"megaohms"};

static const FisicsUnitDef kLengthUnits[] = {
    {"meter", "m", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 1.0, 0.0, kMeterAliases, 3},
    {"millimeter", "mm", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 0.001, 0.0, kMillimeterAliases, 3},
    {"centimeter", "cm", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 0.01, 0.0, kCentimeterAliases, 3},
    {"kilometer", "km", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 1000.0, 0.0, kKilometerAliases, 3},
    {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 0.3048, 0.0, kFootAliases, 1},
    {"inch", "in", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 0.0254, 0.0, kInchAliases, 1},
    {"yard", "yd", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 0.9144, 0.0, kYardAliases, 1},
    {"mile", "mi", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 1609.344, 0.0, kMileAliases, 1}
};

static const FisicsUnitDef kMassUnits[] = {
    {"kilogram", "kg", FISICS_DIM_FAMILY_MASS, {{0, 1, 0, 0, 0, 0, 0, 0}}, 1.0, 0.0, kKilogramAliases, 1},
    {"gram", "g", FISICS_DIM_FAMILY_MASS, {{0, 1, 0, 0, 0, 0, 0, 0}}, 0.001, 0.0, kGramAliases, 1},
    {"milligram", "mg", FISICS_DIM_FAMILY_MASS, {{0, 1, 0, 0, 0, 0, 0, 0}}, 0.000001, 0.0, kMilligramAliases, 1},
    {"metric_ton", "t", FISICS_DIM_FAMILY_MASS, {{0, 1, 0, 0, 0, 0, 0, 0}}, 1000.0, 0.0, kMetricTonAliases, 3},
    {"pound", "lb", FISICS_DIM_FAMILY_MASS, {{0, 1, 0, 0, 0, 0, 0, 0}}, 0.45359237, 0.0, kPoundAliases, 1}
};

static const FisicsUnitDef kTimeUnits[] = {
    {"second", "s", FISICS_DIM_FAMILY_TIME, {{0, 0, 1, 0, 0, 0, 0, 0}}, 1.0, 0.0, kSecondAliases, 1},
    {"millisecond", "ms", FISICS_DIM_FAMILY_TIME, {{0, 0, 1, 0, 0, 0, 0, 0}}, 0.001, 0.0, kMillisecondAliases, 1},
    {"minute", "min", FISICS_DIM_FAMILY_TIME, {{0, 0, 1, 0, 0, 0, 0, 0}}, 60.0, 0.0, kMinuteAliases, 1},
    {"hour", "h", FISICS_DIM_FAMILY_TIME, {{0, 0, 1, 0, 0, 0, 0, 0}}, 3600.0, 0.0, kHourAliases, 1}
};

static const FisicsUnitDef kCurrentUnits[] = {
    {"ampere", "A", FISICS_DIM_FAMILY_CURRENT, {{0, 0, 0, 1, 0, 0, 0, 0}}, 1.0, 0.0, kAmpereAliases, 3}
};

static const FisicsUnitDef kTemperatureUnits[] = {
    {"kelvin", "K", FISICS_DIM_FAMILY_TEMPERATURE, {{0, 0, 0, 0, 1, 0, 0, 0}}, 1.0, 0.0, kKelvinAliases, 1}
};

static const FisicsUnitDef kAmountUnits[] = {
    {"mole", "mol", FISICS_DIM_FAMILY_AMOUNT, {{0, 0, 0, 0, 0, 1, 0, 0}}, 1.0, 0.0, kMoleAliases, 1}
};

static const FisicsUnitDef kLuminousUnits[] = {
    {"candela", "cd", FISICS_DIM_FAMILY_LUMINOUS, {{0, 0, 0, 0, 0, 0, 1, 0}}, 1.0, 0.0, kCandelaAliases, 1}
};

static const FisicsUnitDef kVelocityUnits[] = {
    {"meter_per_second", "m/s", FISICS_DIM_FAMILY_VELOCITY, {{1, 0, -1, 0, 0, 0, 0, 0}}, 1.0, 0.0, kMeterPerSecondAliases, 1},
    {"foot_per_second", "ft/s", FISICS_DIM_FAMILY_VELOCITY, {{1, 0, -1, 0, 0, 0, 0, 0}}, 0.3048, 0.0, kFootPerSecondAliases, 1}
};

static const FisicsUnitDef kAccelerationUnits[] = {
    {"meter_per_second_squared", "m/s^2", FISICS_DIM_FAMILY_ACCELERATION, {{1, 0, -2, 0, 0, 0, 0, 0}}, 1.0, 0.0, kMeterPerSecondSquaredAliases, 1}
};

static const FisicsUnitDef kForceUnits[] = {
    {"newton", "N", FISICS_DIM_FAMILY_FORCE, {{1, 1, -2, 0, 0, 0, 0, 0}}, 1.0, 0.0, kNewtonAliases, 1},
    {"kilonewton", "kN", FISICS_DIM_FAMILY_FORCE, {{1, 1, -2, 0, 0, 0, 0, 0}}, 1000.0, 0.0, kKilonewtonAliases, 1},
    {"pound_force", "lbf", FISICS_DIM_FAMILY_FORCE, {{1, 1, -2, 0, 0, 0, 0, 0}}, 4.4482216152605, 0.0, kPoundForceAliases, 1}
};

static const FisicsUnitDef kEnergyUnits[] = {
    {"joule", "J", FISICS_DIM_FAMILY_ENERGY, {{2, 1, -2, 0, 0, 0, 0, 0}}, 1.0, 0.0, kJouleAliases, 1},
    {"kilojoule", "kJ", FISICS_DIM_FAMILY_ENERGY, {{2, 1, -2, 0, 0, 0, 0, 0}}, 1000.0, 0.0, kKilojouleAliases, 1},
    {"watt_hour", "Wh", FISICS_DIM_FAMILY_ENERGY, {{2, 1, -2, 0, 0, 0, 0, 0}}, 3600.0, 0.0, kWattHourAliases, 1},
    {"kilowatt_hour", "kWh", FISICS_DIM_FAMILY_ENERGY, {{2, 1, -2, 0, 0, 0, 0, 0}}, 3600000.0, 0.0, kKilowattHourAliases, 1}
};

static const FisicsUnitDef kPowerUnits[] = {
    {"watt", "W", FISICS_DIM_FAMILY_POWER, {{2, 1, -3, 0, 0, 0, 0, 0}}, 1.0, 0.0, kWattAliases, 1},
    {"milliwatt", "mW", FISICS_DIM_FAMILY_POWER, {{2, 1, -3, 0, 0, 0, 0, 0}}, 0.001, 0.0, kMilliwattAliases, 1},
    {"kilowatt", "kW", FISICS_DIM_FAMILY_POWER, {{2, 1, -3, 0, 0, 0, 0, 0}}, 1000.0, 0.0, kKilowattAliases, 1},
    {"megawatt", "MW", FISICS_DIM_FAMILY_POWER, {{2, 1, -3, 0, 0, 0, 0, 0}}, 1000000.0, 0.0, kMegawattAliases, 1}
};

static const FisicsUnitDef kPressureUnits[] = {
    {"pascal", "Pa", FISICS_DIM_FAMILY_PRESSURE, {{-1, 1, -2, 0, 0, 0, 0, 0}}, 1.0, 0.0, kPascalAliases, 1},
    {"kilopascal", "kPa", FISICS_DIM_FAMILY_PRESSURE, {{-1, 1, -2, 0, 0, 0, 0, 0}}, 1000.0, 0.0, kKilopascalAliases, 1},
    {"bar", "bar", FISICS_DIM_FAMILY_PRESSURE, {{-1, 1, -2, 0, 0, 0, 0, 0}}, 100000.0, 0.0, kBarAliases, 1},
    {"psi", "psi", FISICS_DIM_FAMILY_PRESSURE, {{-1, 1, -2, 0, 0, 0, 0, 0}}, 6894.757293168361, 0.0, kPsiAliases, 1}
};

static const FisicsUnitDef kChargeUnits[] = {
    {"coulomb", "C", FISICS_DIM_FAMILY_CHARGE, {{0, 0, 1, 1, 0, 0, 0, 0}}, 1.0, 0.0, kCoulombAliases, 1},
    {"ampere_hour", "Ah", FISICS_DIM_FAMILY_CHARGE, {{0, 0, 1, 1, 0, 0, 0, 0}}, 3600.0, 0.0, kAmpereHourAliases, 1},
    {"milliampere_hour", "mAh", FISICS_DIM_FAMILY_CHARGE, {{0, 0, 1, 1, 0, 0, 0, 0}}, 3.6, 0.0, kMilliampereHourAliases, 1}
};

static const FisicsUnitDef kVoltageUnits[] = {
    {"volt", "V", FISICS_DIM_FAMILY_VOLTAGE, {{2, 1, -3, -1, 0, 0, 0, 0}}, 1.0, 0.0, kVoltAliases, 1},
    {"millivolt", "mV", FISICS_DIM_FAMILY_VOLTAGE, {{2, 1, -3, -1, 0, 0, 0, 0}}, 0.001, 0.0, kMillivoltAliases, 1},
    {"kilovolt", "kV", FISICS_DIM_FAMILY_VOLTAGE, {{2, 1, -3, -1, 0, 0, 0, 0}}, 1000.0, 0.0, kKilovoltAliases, 1}
};

static const FisicsUnitDef kResistanceUnits[] = {
    {"ohm", "ohm", FISICS_DIM_FAMILY_RESISTANCE, {{2, 1, -3, -2, 0, 0, 0, 0}}, 1.0, 0.0, kOhmAliases, 1},
    {"milliohm", "mOhm", FISICS_DIM_FAMILY_RESISTANCE, {{2, 1, -3, -2, 0, 0, 0, 0}}, 0.001, 0.0, kMilliohmAliases, 1},
    {"kiloohm", "kOhm", FISICS_DIM_FAMILY_RESISTANCE, {{2, 1, -3, -2, 0, 0, 0, 0}}, 1000.0, 0.0, kKiloohmAliases, 1},
    {"megaohm", "MOhm", FISICS_DIM_FAMILY_RESISTANCE, {{2, 1, -3, -2, 0, 0, 0, 0}}, 1000000.0, 0.0, kMegaohmAliases, 1}
};

static const FisicsUnitFamilyDef kUnitFamilies[] = {
    {FISICS_DIM_FAMILY_LENGTH, "length", kLengthUnits, sizeof(kLengthUnits) / sizeof(kLengthUnits[0])},
    {FISICS_DIM_FAMILY_MASS, "mass", kMassUnits, sizeof(kMassUnits) / sizeof(kMassUnits[0])},
    {FISICS_DIM_FAMILY_TIME, "time", kTimeUnits, sizeof(kTimeUnits) / sizeof(kTimeUnits[0])},
    {FISICS_DIM_FAMILY_CURRENT, "current", kCurrentUnits, sizeof(kCurrentUnits) / sizeof(kCurrentUnits[0])},
    {FISICS_DIM_FAMILY_TEMPERATURE, "temperature", kTemperatureUnits, sizeof(kTemperatureUnits) / sizeof(kTemperatureUnits[0])},
    {FISICS_DIM_FAMILY_AMOUNT, "amount", kAmountUnits, sizeof(kAmountUnits) / sizeof(kAmountUnits[0])},
    {FISICS_DIM_FAMILY_LUMINOUS, "luminous", kLuminousUnits, sizeof(kLuminousUnits) / sizeof(kLuminousUnits[0])},
    {FISICS_DIM_FAMILY_VELOCITY, "velocity", kVelocityUnits, sizeof(kVelocityUnits) / sizeof(kVelocityUnits[0])},
    {FISICS_DIM_FAMILY_ACCELERATION, "acceleration", kAccelerationUnits, sizeof(kAccelerationUnits) / sizeof(kAccelerationUnits[0])},
    {FISICS_DIM_FAMILY_FORCE, "force", kForceUnits, sizeof(kForceUnits) / sizeof(kForceUnits[0])},
    {FISICS_DIM_FAMILY_ENERGY, "energy", kEnergyUnits, sizeof(kEnergyUnits) / sizeof(kEnergyUnits[0])},
    {FISICS_DIM_FAMILY_POWER, "power", kPowerUnits, sizeof(kPowerUnits) / sizeof(kPowerUnits[0])},
    {FISICS_DIM_FAMILY_PRESSURE, "pressure", kPressureUnits, sizeof(kPressureUnits) / sizeof(kPressureUnits[0])},
    {FISICS_DIM_FAMILY_CHARGE, "charge", kChargeUnits, sizeof(kChargeUnits) / sizeof(kChargeUnits[0])},
    {FISICS_DIM_FAMILY_VOLTAGE, "voltage", kVoltageUnits, sizeof(kVoltageUnits) / sizeof(kVoltageUnits[0])},
    {FISICS_DIM_FAMILY_RESISTANCE, "resistance", kResistanceUnits, sizeof(kResistanceUnits) / sizeof(kResistanceUnits[0])}
};

static bool same_text(const char* a, const char* b) {
    return a && b && strcmp(a, b) == 0;
}

static bool family_expected_dim(FisicsDimFamily family, FisicsDim8* outDim) {
    if (!outDim) return false;
    switch (family) {
        case FISICS_DIM_FAMILY_LENGTH: *outDim = fisics_dim_length(); return true;
        case FISICS_DIM_FAMILY_MASS: *outDim = fisics_dim_mass(); return true;
        case FISICS_DIM_FAMILY_TIME: *outDim = fisics_dim_time(); return true;
        case FISICS_DIM_FAMILY_CURRENT: *outDim = fisics_dim_current(); return true;
        case FISICS_DIM_FAMILY_TEMPERATURE: *outDim = fisics_dim_temperature(); return true;
        case FISICS_DIM_FAMILY_AMOUNT: *outDim = fisics_dim_amount(); return true;
        case FISICS_DIM_FAMILY_LUMINOUS: *outDim = fisics_dim_luminous(); return true;
        case FISICS_DIM_FAMILY_CUSTOM: *outDim = fisics_dim_custom(); return true;
        case FISICS_DIM_FAMILY_VELOCITY: *outDim = fisics_dim_velocity(); return true;
        case FISICS_DIM_FAMILY_ACCELERATION: *outDim = fisics_dim_acceleration(); return true;
        case FISICS_DIM_FAMILY_FORCE: *outDim = fisics_dim_force(); return true;
        case FISICS_DIM_FAMILY_ENERGY: *outDim = fisics_dim_energy(); return true;
        case FISICS_DIM_FAMILY_POWER: *outDim = fisics_dim_power(); return true;
        case FISICS_DIM_FAMILY_PRESSURE: *outDim = fisics_dim_pressure(); return true;
        case FISICS_DIM_FAMILY_CHARGE: *outDim = fisics_dim_charge(); return true;
        case FISICS_DIM_FAMILY_VOLTAGE: *outDim = fisics_dim_voltage(); return true;
        case FISICS_DIM_FAMILY_RESISTANCE: *outDim = fisics_dim_resistance(); return true;
        default: *outDim = fisics_dim_zero(); return false;
    }
}

static void record_issue(FisicsUnitRegistryAudit* outAudit) {
    if (!outAudit) return;
    outAudit->issue_count += 1;
}

static bool unit_has_canonical_base(const FisicsUnitDef* unit) {
    return unit && unit->scale_to_canonical == 1.0 && unit->offset_to_canonical == 0.0;
}

static void validate_unit_tokens(const FisicsUnitDef* unit, FisicsUnitRegistryAudit* outAudit) {
    if (!unit) {
        record_issue(outAudit);
        return;
    }
    if (!unit->name || unit->name[0] == '\0') record_issue(outAudit);
    if (!unit->symbol || unit->symbol[0] == '\0') record_issue(outAudit);
    for (size_t alias_index = 0; alias_index < unit->alias_count; ++alias_index) {
        const char* alias = unit->aliases ? unit->aliases[alias_index] : NULL;
        if (!alias || alias[0] == '\0') {
            record_issue(outAudit);
            continue;
        }
        if (same_text(alias, unit->name) || same_text(alias, unit->symbol)) {
            record_issue(outAudit);
        }
    }
}

static void validate_cross_collisions(const FisicsUnitFamilyDef* families,
                                      size_t family_count,
                                      FisicsUnitRegistryAudit* outAudit) {
    for (size_t family_index = 0; family_index < family_count; ++family_index) {
        const FisicsUnitFamilyDef* family = &families[family_index];
        for (size_t unit_index = 0; unit_index < family->unit_count; ++unit_index) {
            const FisicsUnitDef* unit = &family->units[unit_index];
            for (size_t compare_family_index = family_index; compare_family_index < family_count; ++compare_family_index) {
                const FisicsUnitFamilyDef* compare_family = &families[compare_family_index];
                size_t compare_start = (compare_family_index == family_index) ? unit_index + 1 : 0;
                for (size_t compare_unit_index = compare_start;
                     compare_unit_index < compare_family->unit_count;
                     ++compare_unit_index) {
                    const FisicsUnitDef* compare_unit = &compare_family->units[compare_unit_index];

                    if (same_text(unit->name, compare_unit->name) ||
                        same_text(unit->name, compare_unit->symbol) ||
                        same_text(unit->symbol, compare_unit->name) ||
                        same_text(unit->symbol, compare_unit->symbol)) {
                        record_issue(outAudit);
                    }

                    for (size_t alias_index = 0; alias_index < unit->alias_count; ++alias_index) {
                        const char* alias = unit->aliases ? unit->aliases[alias_index] : NULL;
                        if (!alias) continue;
                        if (same_text(alias, compare_unit->name) || same_text(alias, compare_unit->symbol)) {
                            record_issue(outAudit);
                        }
                        for (size_t compare_alias_index = 0;
                             compare_alias_index < compare_unit->alias_count;
                             ++compare_alias_index) {
                            const char* compare_alias =
                                compare_unit->aliases ? compare_unit->aliases[compare_alias_index] : NULL;
                            if (same_text(alias, compare_alias)) {
                                record_issue(outAudit);
                            }
                        }
                    }

                    for (size_t compare_alias_index = 0;
                         compare_alias_index < compare_unit->alias_count;
                         ++compare_alias_index) {
                        const char* compare_alias =
                            compare_unit->aliases ? compare_unit->aliases[compare_alias_index] : NULL;
                        if (!compare_alias) continue;
                        if (same_text(compare_alias, unit->name) || same_text(compare_alias, unit->symbol)) {
                            record_issue(outAudit);
                        }
                    }
                }
            }
        }
    }
}

bool fisics_unit_lookup(const char* name, const FisicsUnitDef** outUnit) {
    if (!name || !outUnit) return false;
    *outUnit = NULL;

    for (size_t i = 0; i < sizeof(kUnitFamilies) / sizeof(kUnitFamilies[0]); ++i) {
        const FisicsUnitFamilyDef* family = &kUnitFamilies[i];
        for (size_t j = 0; j < family->unit_count; ++j) {
            const FisicsUnitDef* unit = &family->units[j];
            if (strcmp(unit->name, name) == 0 || strcmp(unit->symbol, name) == 0) {
                *outUnit = unit;
                return true;
            }
            for (size_t k = 0; k < unit->alias_count; ++k) {
                if (strcmp(unit->aliases[k], name) == 0) {
                    *outUnit = unit;
                    return true;
                }
            }
        }
    }

    return false;
}

bool fisics_unit_lookup_family(FisicsDimFamily family, const FisicsUnitFamilyDef** outFamily) {
    if (!outFamily) return false;
    *outFamily = NULL;
    for (size_t i = 0; i < sizeof(kUnitFamilies) / sizeof(kUnitFamilies[0]); ++i) {
        if (kUnitFamilies[i].family != family) continue;
        *outFamily = &kUnitFamilies[i];
        return true;
    }
    return false;
}

size_t fisics_unit_family_count(void) {
    return sizeof(kUnitFamilies) / sizeof(kUnitFamilies[0]);
}

const FisicsUnitFamilyDef* fisics_unit_family_at(size_t index) {
    if (index >= fisics_unit_family_count()) return NULL;
    return &kUnitFamilies[index];
}

bool fisics_unit_registry_validate_table(const FisicsUnitFamilyDef* families,
                                         size_t family_count,
                                         FisicsUnitRegistryAudit* outAudit) {
    FisicsUnitRegistryAudit audit = {0};
    if (!families || family_count == 0) {
        audit.issue_count = 1;
        if (outAudit) *outAudit = audit;
        return false;
    }

    for (size_t family_index = 0; family_index < family_count; ++family_index) {
        const FisicsUnitFamilyDef* family = &families[family_index];
        FisicsDim8 expected_dim = fisics_dim_zero();
        bool have_expected_dim = family_expected_dim(family->family, &expected_dim);
        size_t canonical_base_count = 0;

        audit.family_count += 1;
        if (!family->family_name || family->family_name[0] == '\0') {
            record_issue(&audit);
        } else if (!same_text(family->family_name, fisics_dim_family_name(family->family))) {
            record_issue(&audit);
        }
        if (!family->units || family->unit_count == 0) {
            record_issue(&audit);
            continue;
        }

        for (size_t unit_index = 0; unit_index < family->unit_count; ++unit_index) {
            const FisicsUnitDef* unit = &family->units[unit_index];
            audit.unit_count += 1;
            audit.alias_count += unit->alias_count;

            validate_unit_tokens(unit, &audit);

            if (unit->family != family->family) {
                record_issue(&audit);
            }
            if (!have_expected_dim || !fisics_dim_equal(unit->dim, expected_dim)) {
                record_issue(&audit);
            }
            if (!isfinite(unit->scale_to_canonical) || unit->scale_to_canonical <= 0.0) {
                record_issue(&audit);
            }
            if (!isfinite(unit->offset_to_canonical) || unit->offset_to_canonical != 0.0) {
                record_issue(&audit);
            }
            if (unit_has_canonical_base(unit)) {
                canonical_base_count += 1;
            }
        }

        if (canonical_base_count != 1) {
            record_issue(&audit);
        }
    }

    validate_cross_collisions(families, family_count, &audit);

    if (outAudit) *outAudit = audit;
    return audit.issue_count == 0;
}

bool fisics_unit_registry_validate(FisicsUnitRegistryAudit* outAudit) {
    return fisics_unit_registry_validate_table(
        kUnitFamilies,
        sizeof(kUnitFamilies) / sizeof(kUnitFamilies[0]),
        outAudit);
}
