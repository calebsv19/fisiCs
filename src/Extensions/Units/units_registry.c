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
static const char* const kFootAliases[] = {"feet", "feet"};
static const char* const kInchAliases[] = {"inches", "inches"};
static const char* const kYardAliases[] = {"yards", "yards"};
static const char* const kMileAliases[] = {"miles", "miles"};
static const char* const kKilogramAliases[] = {"kilograms", "kilograms"};
static const char* const kGramAliases[] = {"grams", "grams"};
static const char* const kMilligramAliases[] = {"milligrams", "milligrams"};
static const char* const kMetricTonAliases[] = {"metric_tons", "tonne", "tonnes"};
static const char* const kPoundAliases[] = {"pounds", "pounds"};
static const char* const kSecondAliases[] = {"seconds", "seconds"};
static const char* const kMillisecondAliases[] = {"milliseconds", "milliseconds"};
static const char* const kMinuteAliases[] = {"minutes", "minutes"};
static const char* const kHourAliases[] = {"hours", "hours"};
static const char* const kAmpereAliases[] = {"amp", "amps", "amperes"};
static const char* const kKelvinAliases[] = {"kelvins", "kelvins"};
static const char* const kMoleAliases[] = {"moles", "moles"};
static const char* const kCandelaAliases[] = {"candelas", "candelas"};
static const char* const kMeterPerSecondAliases[] = {"meters_per_second", "meters_per_second"};
static const char* const kFootPerSecondAliases[] = {"feet_per_second", "feet_per_second"};
static const char* const kMeterPerSecondSquaredAliases[] = {"meters_per_second_squared", "meters_per_second_squared"};
static const char* const kNewtonAliases[] = {"newtons", "newtons"};
static const char* const kKilonewtonAliases[] = {"kilonewtons", "kilonewtons"};
static const char* const kPoundForceAliases[] = {"pound_forces", "pound_forces"};
static const char* const kJouleAliases[] = {"joules", "joules"};
static const char* const kKilojouleAliases[] = {"kilojoules", "kilojoules"};
static const char* const kWattHourAliases[] = {"watt_hours", "watt_hours"};
static const char* const kKilowattHourAliases[] = {"kilowatt_hours", "kilowatt_hours"};
static const char* const kWattAliases[] = {"watts", "watts"};
static const char* const kMilliwattAliases[] = {"milliwatts", "milliwatts"};
static const char* const kKilowattAliases[] = {"kilowatts", "kilowatts"};
static const char* const kMegawattAliases[] = {"megawatts", "megawatts"};
static const char* const kPascalAliases[] = {"pascals", "pascals"};
static const char* const kKilopascalAliases[] = {"kilopascals", "kilopascals"};
static const char* const kBarAliases[] = {"bars", "bars"};
static const char* const kPsiAliases[] = {"psis", "psis"};
static const char* const kCoulombAliases[] = {"coulombs", "coulombs"};
static const char* const kAmpereHourAliases[] = {"ampere_hours", "ampere_hours"};
static const char* const kMilliampereHourAliases[] = {"milliampere_hours", "milliampere_hours"};
static const char* const kVoltAliases[] = {"volts", "volts"};
static const char* const kMillivoltAliases[] = {"millivolts", "millivolts"};
static const char* const kKilovoltAliases[] = {"kilovolts", "kilovolts"};
static const char* const kOhmAliases[] = {"ohms", "ohms"};
static const char* const kMilliohmAliases[] = {"milliohms", "milliohms"};
static const char* const kKiloohmAliases[] = {"kiloohms", "kiloohms"};
static const char* const kMegaohmAliases[] = {"megaohms", "megaohms"};

#define ASSIGN_UNIT(slot, unit_name, unit_symbol, unit_family, e0, e1, e2, e3, e4, e5, e6, e7, scale_value, alias_array) \
    do {                                                                                                                     \
        (slot).name = (unit_name);                                                                                           \
        (slot).symbol = (unit_symbol);                                                                                       \
        (slot).family = (unit_family);                                                                                       \
        (slot).dim.e[0] = (e0);                                                                                              \
        (slot).dim.e[1] = (e1);                                                                                              \
        (slot).dim.e[2] = (e2);                                                                                              \
        (slot).dim.e[3] = (e3);                                                                                              \
        (slot).dim.e[4] = (e4);                                                                                              \
        (slot).dim.e[5] = (e5);                                                                                              \
        (slot).dim.e[6] = (e6);                                                                                              \
        (slot).dim.e[7] = (e7);                                                                                              \
        (slot).scale_to_canonical = (scale_value);                                                                           \
        (slot).offset_to_canonical = 0.0;                                                                                    \
        (slot).aliases = (alias_array);                                                                                      \
        (slot).alias_count = sizeof(alias_array) / sizeof((alias_array)[0]);                                                 \
    } while (0)

#define ASSIGN_FAMILY(slot, family_id, label, units_array)            \
    do {                                                              \
        (slot).family = (family_id);                                  \
        (slot).family_name = (label);                                 \
        (slot).units = (units_array);                                 \
        (slot).unit_count = sizeof(units_array) / sizeof((units_array)[0]); \
    } while (0)

#define UNIT_DEF_INIT(unit_name, unit_symbol, unit_family, e0, e1, e2, e3, e4, e5, e6, e7, scale_value, alias_array) \
    {                                                                                                                 \
        unit_name,                                                                                                    \
        unit_symbol,                                                                                                  \
        unit_family,                                                                                                  \
        {{e0, e1, e2, e3, e4, e5, e6, e7}},                                                                          \
        scale_value,                                                                                                  \
        0.0,                                                                                                          \
        alias_array,                                                                                                  \
        sizeof(alias_array) / sizeof((alias_array)[0])                                                                \
    }

static const FisicsUnitDef kCurrentUnitsTemplate[1] = {
    UNIT_DEF_INIT("ampere", "A", FISICS_DIM_FAMILY_CURRENT, 0, 0, 0, 1, 0, 0, 0, 0, 1.0, kAmpereAliases),
};

static const FisicsUnitDef kTemperatureUnitsTemplate[1] = {
    UNIT_DEF_INIT("kelvin", "K", FISICS_DIM_FAMILY_TEMPERATURE, 0, 0, 0, 0, 1, 0, 0, 0, 1.0, kKelvinAliases),
};

static const FisicsUnitDef kAmountUnitsTemplate[1] = {
    UNIT_DEF_INIT("mole", "mol", FISICS_DIM_FAMILY_AMOUNT, 0, 0, 0, 0, 0, 1, 0, 0, 1.0, kMoleAliases),
};

static const FisicsUnitDef kLuminousUnitsTemplate[1] = {
    UNIT_DEF_INIT("candela", "cd", FISICS_DIM_FAMILY_LUMINOUS, 0, 0, 0, 0, 0, 0, 1, 0, 1.0, kCandelaAliases),
};

static const FisicsUnitDef kForceUnitsTemplate[3] = {
    UNIT_DEF_INIT("newton", "N", FISICS_DIM_FAMILY_FORCE, 1, 1, -2, 0, 0, 0, 0, 0, 1.0, kNewtonAliases),
    UNIT_DEF_INIT("kilonewton", "kN", FISICS_DIM_FAMILY_FORCE, 1, 1, -2, 0, 0, 0, 0, 0, 1000.0, kKilonewtonAliases),
    UNIT_DEF_INIT("pound_force", "lbf", FISICS_DIM_FAMILY_FORCE, 1, 1, -2, 0, 0, 0, 0, 0, 4.4482216152605, kPoundForceAliases),
};

static const FisicsUnitDef kPowerUnitsTemplate[4] = {
    UNIT_DEF_INIT("watt", "W", FISICS_DIM_FAMILY_POWER, 2, 1, -3, 0, 0, 0, 0, 0, 1.0, kWattAliases),
    UNIT_DEF_INIT("milliwatt", "mW", FISICS_DIM_FAMILY_POWER, 2, 1, -3, 0, 0, 0, 0, 0, 0.001, kMilliwattAliases),
    UNIT_DEF_INIT("kilowatt", "kW", FISICS_DIM_FAMILY_POWER, 2, 1, -3, 0, 0, 0, 0, 0, 1000.0, kKilowattAliases),
    UNIT_DEF_INIT("megawatt", "MW", FISICS_DIM_FAMILY_POWER, 2, 1, -3, 0, 0, 0, 0, 0, 1000000.0, kMegawattAliases),
};

static const FisicsUnitDef kPressureUnitsTemplate[4] = {
    UNIT_DEF_INIT("pascal", "Pa", FISICS_DIM_FAMILY_PRESSURE, -1, 1, -2, 0, 0, 0, 0, 0, 1.0, kPascalAliases),
    UNIT_DEF_INIT("kilopascal", "kPa", FISICS_DIM_FAMILY_PRESSURE, -1, 1, -2, 0, 0, 0, 0, 0, 1000.0, kKilopascalAliases),
    UNIT_DEF_INIT("bar", "bar", FISICS_DIM_FAMILY_PRESSURE, -1, 1, -2, 0, 0, 0, 0, 0, 100000.0, kBarAliases),
    UNIT_DEF_INIT("psi", "psi", FISICS_DIM_FAMILY_PRESSURE, -1, 1, -2, 0, 0, 0, 0, 0, 6894.757293168361, kPsiAliases),
};

static const FisicsUnitDef kChargeUnitsTemplate[3] = {
    UNIT_DEF_INIT("coulomb", "C", FISICS_DIM_FAMILY_CHARGE, 0, 0, 1, 1, 0, 0, 0, 0, 1.0, kCoulombAliases),
    UNIT_DEF_INIT("ampere_hour", "Ah", FISICS_DIM_FAMILY_CHARGE, 0, 0, 1, 1, 0, 0, 0, 0, 3600.0, kAmpereHourAliases),
    UNIT_DEF_INIT("milliampere_hour", "mAh", FISICS_DIM_FAMILY_CHARGE, 0, 0, 1, 1, 0, 0, 0, 0, 3.6, kMilliampereHourAliases),
};

static const FisicsUnitDef kVoltageUnitsTemplate[3] = {
    UNIT_DEF_INIT("volt", "V", FISICS_DIM_FAMILY_VOLTAGE, 2, 1, -3, -1, 0, 0, 0, 0, 1.0, kVoltAliases),
    UNIT_DEF_INIT("millivolt", "mV", FISICS_DIM_FAMILY_VOLTAGE, 2, 1, -3, -1, 0, 0, 0, 0, 0.001, kMillivoltAliases),
    UNIT_DEF_INIT("kilovolt", "kV", FISICS_DIM_FAMILY_VOLTAGE, 2, 1, -3, -1, 0, 0, 0, 0, 1000.0, kKilovoltAliases),
};

static const FisicsUnitDef kResistanceUnitsTemplate[4] = {
    UNIT_DEF_INIT("ohm", "ohm", FISICS_DIM_FAMILY_RESISTANCE, 2, 1, -3, -2, 0, 0, 0, 0, 1.0, kOhmAliases),
    UNIT_DEF_INIT("milliohm", "mOhm", FISICS_DIM_FAMILY_RESISTANCE, 2, 1, -3, -2, 0, 0, 0, 0, 0.001, kMilliohmAliases),
    UNIT_DEF_INIT("kiloohm", "kOhm", FISICS_DIM_FAMILY_RESISTANCE, 2, 1, -3, -2, 0, 0, 0, 0, 1000.0, kKiloohmAliases),
    UNIT_DEF_INIT("megaohm", "MOhm", FISICS_DIM_FAMILY_RESISTANCE, 2, 1, -3, -2, 0, 0, 0, 0, 1000000.0, kMegaohmAliases),
};

static FisicsUnitDef kLengthUnits[8];
static FisicsUnitDef kMassUnits[5];
static FisicsUnitDef kTimeUnits[4];
static FisicsUnitDef kCurrentUnits[1];
static FisicsUnitDef kTemperatureUnits[1];
static FisicsUnitDef kAmountUnits[1];
static FisicsUnitDef kLuminousUnits[1];
static FisicsUnitDef kVelocityUnits[2];
static FisicsUnitDef kAccelerationUnits[1];
static FisicsUnitDef kForceUnits[3];
static FisicsUnitDef kEnergyUnits[4];
static FisicsUnitDef kPowerUnits[4];
static FisicsUnitDef kPressureUnits[4];
static FisicsUnitDef kChargeUnits[3];
static FisicsUnitDef kVoltageUnits[3];
static FisicsUnitDef kResistanceUnits[4];
static FisicsUnitFamilyDef kUnitFamilies[16];
static bool kUnitRegistryInitialized = false;

static void init_length_units_head(void) {
    ASSIGN_UNIT(kLengthUnits[0], "meter", "m", FISICS_DIM_FAMILY_LENGTH, 1, 0, 0, 0, 0, 0, 0, 0, 1.0, kMeterAliases);
    ASSIGN_UNIT(kLengthUnits[1], "millimeter", "mm", FISICS_DIM_FAMILY_LENGTH, 1, 0, 0, 0, 0, 0, 0, 0, 0.001, kMillimeterAliases);
    ASSIGN_UNIT(kLengthUnits[2], "centimeter", "cm", FISICS_DIM_FAMILY_LENGTH, 1, 0, 0, 0, 0, 0, 0, 0, 0.01, kCentimeterAliases);
    ASSIGN_UNIT(kLengthUnits[3], "kilometer", "km", FISICS_DIM_FAMILY_LENGTH, 1, 0, 0, 0, 0, 0, 0, 0, 1000.0, kKilometerAliases);
}

static void init_length_units_tail(void) {
    ASSIGN_UNIT(kLengthUnits[4], "foot", "ft", FISICS_DIM_FAMILY_LENGTH, 1, 0, 0, 0, 0, 0, 0, 0, 0.3048, kFootAliases);
    ASSIGN_UNIT(kLengthUnits[5], "inch", "in", FISICS_DIM_FAMILY_LENGTH, 1, 0, 0, 0, 0, 0, 0, 0, 0.0254, kInchAliases);
    ASSIGN_UNIT(kLengthUnits[6], "yard", "yd", FISICS_DIM_FAMILY_LENGTH, 1, 0, 0, 0, 0, 0, 0, 0, 0.9144, kYardAliases);
    ASSIGN_UNIT(kLengthUnits[7], "mile", "mi", FISICS_DIM_FAMILY_LENGTH, 1, 0, 0, 0, 0, 0, 0, 0, 1609.344, kMileAliases);
}

static void init_length_units(void) {
    init_length_units_head();
    init_length_units_tail();
}

static void init_mass_units(void) {
    ASSIGN_UNIT(kMassUnits[0], "kilogram", "kg", FISICS_DIM_FAMILY_MASS, 0, 1, 0, 0, 0, 0, 0, 0, 1.0, kKilogramAliases);
    ASSIGN_UNIT(kMassUnits[1], "gram", "g", FISICS_DIM_FAMILY_MASS, 0, 1, 0, 0, 0, 0, 0, 0, 0.001, kGramAliases);
    ASSIGN_UNIT(kMassUnits[2], "milligram", "mg", FISICS_DIM_FAMILY_MASS, 0, 1, 0, 0, 0, 0, 0, 0, 0.000001, kMilligramAliases);
    ASSIGN_UNIT(kMassUnits[3], "metric_ton", "t", FISICS_DIM_FAMILY_MASS, 0, 1, 0, 0, 0, 0, 0, 0, 1000.0, kMetricTonAliases);
    ASSIGN_UNIT(kMassUnits[4], "pound", "lb", FISICS_DIM_FAMILY_MASS, 0, 1, 0, 0, 0, 0, 0, 0, 0.45359237, kPoundAliases);
}

static void init_time_units(void) {
    ASSIGN_UNIT(kTimeUnits[0], "second", "s", FISICS_DIM_FAMILY_TIME, 0, 0, 1, 0, 0, 0, 0, 0, 1.0, kSecondAliases);
    ASSIGN_UNIT(kTimeUnits[1], "millisecond", "ms", FISICS_DIM_FAMILY_TIME, 0, 0, 1, 0, 0, 0, 0, 0, 0.001, kMillisecondAliases);
    ASSIGN_UNIT(kTimeUnits[2], "minute", "min", FISICS_DIM_FAMILY_TIME, 0, 0, 1, 0, 0, 0, 0, 0, 60.0, kMinuteAliases);
    ASSIGN_UNIT(kTimeUnits[3], "hour", "h", FISICS_DIM_FAMILY_TIME, 0, 0, 1, 0, 0, 0, 0, 0, 3600.0, kHourAliases);
}

static void init_current_units(void) {
    memcpy(kCurrentUnits, kCurrentUnitsTemplate, sizeof(kCurrentUnits));
}

static void init_temperature_units(void) {
    memcpy(kTemperatureUnits, kTemperatureUnitsTemplate, sizeof(kTemperatureUnits));
}

static void init_amount_units(void) {
    memcpy(kAmountUnits, kAmountUnitsTemplate, sizeof(kAmountUnits));
}

static void init_luminous_units(void) {
    memcpy(kLuminousUnits, kLuminousUnitsTemplate, sizeof(kLuminousUnits));
}

static void init_velocity_units(void) {
    ASSIGN_UNIT(kVelocityUnits[0], "meter_per_second", "m/s", FISICS_DIM_FAMILY_VELOCITY, 1, 0, -1, 0, 0, 0, 0, 0, 1.0, kMeterPerSecondAliases);
    ASSIGN_UNIT(kVelocityUnits[1], "foot_per_second", "ft/s", FISICS_DIM_FAMILY_VELOCITY, 1, 0, -1, 0, 0, 0, 0, 0, 0.3048, kFootPerSecondAliases);
}

static void init_acceleration_units(void) {
    ASSIGN_UNIT(kAccelerationUnits[0], "meter_per_second_squared", "m/s^2", FISICS_DIM_FAMILY_ACCELERATION, 1, 0, -2, 0, 0, 0, 0, 0, 1.0, kMeterPerSecondSquaredAliases);
}

static void init_force_units(void) {
    memcpy(kForceUnits, kForceUnitsTemplate, sizeof(kForceUnits));
}

static void init_energy_units(void) {
    ASSIGN_UNIT(kEnergyUnits[0], "joule", "J", FISICS_DIM_FAMILY_ENERGY, 2, 1, -2, 0, 0, 0, 0, 0, 1.0, kJouleAliases);
    ASSIGN_UNIT(kEnergyUnits[1], "kilojoule", "kJ", FISICS_DIM_FAMILY_ENERGY, 2, 1, -2, 0, 0, 0, 0, 0, 1000.0, kKilojouleAliases);
    ASSIGN_UNIT(kEnergyUnits[2], "watt_hour", "Wh", FISICS_DIM_FAMILY_ENERGY, 2, 1, -2, 0, 0, 0, 0, 0, 3600.0, kWattHourAliases);
    ASSIGN_UNIT(kEnergyUnits[3], "kilowatt_hour", "kWh", FISICS_DIM_FAMILY_ENERGY, 2, 1, -2, 0, 0, 0, 0, 0, 3600000.0, kKilowattHourAliases);
}

static void init_power_units(void) {
    memcpy(kPowerUnits, kPowerUnitsTemplate, sizeof(kPowerUnits));
}

static void init_pressure_units(void) {
    memcpy(kPressureUnits, kPressureUnitsTemplate, sizeof(kPressureUnits));
}

static void init_charge_units(void) {
    memcpy(kChargeUnits, kChargeUnitsTemplate, sizeof(kChargeUnits));
}

static void init_voltage_units(void) {
    memcpy(kVoltageUnits, kVoltageUnitsTemplate, sizeof(kVoltageUnits));
}

static void init_resistance_units(void) {
    memcpy(kResistanceUnits, kResistanceUnitsTemplate, sizeof(kResistanceUnits));
}

static void init_registry_stage1(void) {
    init_length_units();
    init_mass_units();
    init_time_units();
    init_current_units();
}

static void init_registry_stage2(void) {
    init_temperature_units();
    init_amount_units();
    init_luminous_units();
    init_velocity_units();
}

static void init_registry_stage3(void) {
    init_acceleration_units();
    init_force_units();
    init_energy_units();
    init_power_units();
}

static void init_registry_stage4(void) {
    init_pressure_units();
    init_charge_units();
    init_voltage_units();
    init_resistance_units();
}

static void init_family_table_part1(void) {
    ASSIGN_FAMILY(kUnitFamilies[0], FISICS_DIM_FAMILY_LENGTH, "length", kLengthUnits);
    ASSIGN_FAMILY(kUnitFamilies[1], FISICS_DIM_FAMILY_MASS, "mass", kMassUnits);
    ASSIGN_FAMILY(kUnitFamilies[2], FISICS_DIM_FAMILY_TIME, "time", kTimeUnits);
    ASSIGN_FAMILY(kUnitFamilies[3], FISICS_DIM_FAMILY_CURRENT, "current", kCurrentUnits);
    ASSIGN_FAMILY(kUnitFamilies[4], FISICS_DIM_FAMILY_TEMPERATURE, "temperature", kTemperatureUnits);
    ASSIGN_FAMILY(kUnitFamilies[5], FISICS_DIM_FAMILY_AMOUNT, "amount", kAmountUnits);
    ASSIGN_FAMILY(kUnitFamilies[6], FISICS_DIM_FAMILY_LUMINOUS, "luminous", kLuminousUnits);
    ASSIGN_FAMILY(kUnitFamilies[7], FISICS_DIM_FAMILY_VELOCITY, "velocity", kVelocityUnits);
}

static void init_family_table_part2(void) {
    ASSIGN_FAMILY(kUnitFamilies[8], FISICS_DIM_FAMILY_ACCELERATION, "acceleration", kAccelerationUnits);
    ASSIGN_FAMILY(kUnitFamilies[9], FISICS_DIM_FAMILY_FORCE, "force", kForceUnits);
    ASSIGN_FAMILY(kUnitFamilies[10], FISICS_DIM_FAMILY_ENERGY, "energy", kEnergyUnits);
    ASSIGN_FAMILY(kUnitFamilies[11], FISICS_DIM_FAMILY_POWER, "power", kPowerUnits);
    ASSIGN_FAMILY(kUnitFamilies[12], FISICS_DIM_FAMILY_PRESSURE, "pressure", kPressureUnits);
    ASSIGN_FAMILY(kUnitFamilies[13], FISICS_DIM_FAMILY_CHARGE, "charge", kChargeUnits);
    ASSIGN_FAMILY(kUnitFamilies[14], FISICS_DIM_FAMILY_VOLTAGE, "voltage", kVoltageUnits);
    ASSIGN_FAMILY(kUnitFamilies[15], FISICS_DIM_FAMILY_RESISTANCE, "resistance", kResistanceUnits);
}

static void init_registry_stage5(void) {
    init_family_table_part1();
    init_family_table_part2();
}

static void ensure_unit_registry_initialized(void) {
    if (kUnitRegistryInitialized) return;

    init_registry_stage1();
    init_registry_stage2();
    init_registry_stage3();
    init_registry_stage4();
    init_registry_stage5();

    kUnitRegistryInitialized = true;
}

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
    ensure_unit_registry_initialized();

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
    ensure_unit_registry_initialized();
    for (size_t i = 0; i < sizeof(kUnitFamilies) / sizeof(kUnitFamilies[0]); ++i) {
        if (kUnitFamilies[i].family != family) continue;
        *outFamily = &kUnitFamilies[i];
        return true;
    }
    return false;
}

size_t fisics_unit_family_count(void) {
    ensure_unit_registry_initialized();
    return sizeof(kUnitFamilies) / sizeof(kUnitFamilies[0]);
}

const FisicsUnitFamilyDef* fisics_unit_family_at(size_t index) {
    ensure_unit_registry_initialized();
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
    ensure_unit_registry_initialized();
    return fisics_unit_registry_validate_table(
        kUnitFamilies,
        sizeof(kUnitFamilies) / sizeof(kUnitFamilies[0]),
        outAudit);
}
