// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_registry.h"

#include <string.h>

static const char* const kMeterAliases[] = {"meters", "metre", "metres"};
static const char* const kFootAliases[] = {"feet"};
static const char* const kInchAliases[] = {"inches"};
static const char* const kKilogramAliases[] = {"kilograms"};
static const char* const kGramAliases[] = {"grams"};
static const char* const kSecondAliases[] = {"seconds"};
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
static const char* const kJouleAliases[] = {"joules"};
static const char* const kWattAliases[] = {"watts"};
static const char* const kPascalAliases[] = {"pascals"};
static const char* const kCoulombAliases[] = {"coulombs"};
static const char* const kVoltAliases[] = {"volts"};
static const char* const kOhmAliases[] = {"ohms"};

static const FisicsUnitDef kLengthUnits[] = {
    {"meter", "m", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 1.0, 0.0, kMeterAliases, 3},
    {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 0.3048, 0.0, kFootAliases, 1},
    {"inch", "in", FISICS_DIM_FAMILY_LENGTH, {{1, 0, 0, 0, 0, 0, 0, 0}}, 0.0254, 0.0, kInchAliases, 1}
};

static const FisicsUnitDef kMassUnits[] = {
    {"kilogram", "kg", FISICS_DIM_FAMILY_MASS, {{0, 1, 0, 0, 0, 0, 0, 0}}, 1.0, 0.0, kKilogramAliases, 1},
    {"gram", "g", FISICS_DIM_FAMILY_MASS, {{0, 1, 0, 0, 0, 0, 0, 0}}, 0.001, 0.0, kGramAliases, 1}
};

static const FisicsUnitDef kTimeUnits[] = {
    {"second", "s", FISICS_DIM_FAMILY_TIME, {{0, 0, 1, 0, 0, 0, 0, 0}}, 1.0, 0.0, kSecondAliases, 1},
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
    {"newton", "N", FISICS_DIM_FAMILY_FORCE, {{1, 1, -2, 0, 0, 0, 0, 0}}, 1.0, 0.0, kNewtonAliases, 1}
};

static const FisicsUnitDef kEnergyUnits[] = {
    {"joule", "J", FISICS_DIM_FAMILY_ENERGY, {{2, 1, -2, 0, 0, 0, 0, 0}}, 1.0, 0.0, kJouleAliases, 1}
};

static const FisicsUnitDef kPowerUnits[] = {
    {"watt", "W", FISICS_DIM_FAMILY_POWER, {{2, 1, -3, 0, 0, 0, 0, 0}}, 1.0, 0.0, kWattAliases, 1}
};

static const FisicsUnitDef kPressureUnits[] = {
    {"pascal", "Pa", FISICS_DIM_FAMILY_PRESSURE, {{-1, 1, -2, 0, 0, 0, 0, 0}}, 1.0, 0.0, kPascalAliases, 1}
};

static const FisicsUnitDef kChargeUnits[] = {
    {"coulomb", "C", FISICS_DIM_FAMILY_CHARGE, {{0, 0, 1, 1, 0, 0, 0, 0}}, 1.0, 0.0, kCoulombAliases, 1}
};

static const FisicsUnitDef kVoltageUnits[] = {
    {"volt", "V", FISICS_DIM_FAMILY_VOLTAGE, {{2, 1, -3, -1, 0, 0, 0, 0}}, 1.0, 0.0, kVoltAliases, 1}
};

static const FisicsUnitDef kResistanceUnits[] = {
    {"ohm", "ohm", FISICS_DIM_FAMILY_RESISTANCE, {{2, 1, -3, -2, 0, 0, 0, 0}}, 1.0, 0.0, kOhmAliases, 1}
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
