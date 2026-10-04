/* Intake: GrowthSim Fire seeding, runtime_mismatch, real-program discovery.
 * Owner: 14-runtime-surface. Clang's default contraction is legal; the oracle
 * below requires -ffp-contract=off and round-to-nearest, source precision.
 * Known answers are binary32/64 arithmetic, not a tolerance or Clang snapshot.
 */
#include <stdio.h>
#include <math.h>
#ifdef FP_UNITS_CONTROL
#include <fisics/extensions.h>
#else
#define FISICS_DIM(value)
#define FISICS_UNIT(value)
#endif

static float f32(unsigned u) {
    union { unsigned u; float f; } v;
    v.u = u;
    return v.f;
}
static unsigned bits32(float f) {
    union { unsigned u; float f; } v;
    v.f = f;
    return v.u;
}
static double f64(unsigned long long u) {
    union { unsigned long long u; double d; } v;
    v.u = u;
    return v.d;
}
static unsigned long long bits64(double d) {
    union { unsigned long long u; double d; } v;
    v.d = d;
    return v.u;
}
static float seed_mix(float t, float p, float s) {
    return t * 0.45f + p * 0.35f + s * 0.20f;
}
int main(void) {
    volatile unsigned ta = 0x3f16f736u, pa = 0x3f7594d7u, sa = 0x3d4ea668u;
    float t = f32(ta), p = f32(pa), s = f32(sa);
    unsigned first = bits32(t * 0.45f + p * 0.35f);
    unsigned mix = bits32(seed_mix(t, p, s));
    /* The exact first sum lies halfway between 0x3f19e356/357;
     * separate products select the even low bit, a fused product does not. */
    if (first != 0x3f19e356u || mix != 0x3f1c789eu) return 1;
    printf("seed first=%08x mix=%08x\n", first, mix);

    volatile unsigned up = 0x3f800001u, down = 0x3f7ffffeu;
    float a = f32(up), b = f32(down);
    /* (1+2^-23)*(1-2^-23) = 1-2^-46. A separately rounded
     * product is 1; a fused subtraction retains -2^-46. */
    unsigned add = bits32(a * b + -1.0f);
    unsigned sub = bits32(a * b - 1.0f);
    unsigned reverse = bits32(1.0f - a * b);
    float compound = -1.0f;
    compound += a * b;
    float stored = a * b;
    unsigned barrier = bits32(stored - 1.0f);
    unsigned fused = bits32(fmaf(a, b, -1.0f));
    if (add || sub || reverse || bits32(compound) || barrier || fused != 0xa8800000u) return 2;
    printf("float add=%08x sub=%08x reverse=%08x compound=%08x stored=%08x fma=%08x\n",
           add, sub, reverse, bits32(compound), barrier, fused);

    volatile unsigned long long du = 0x3ff0000000000001ull, dd = 0x3feffffffffffffeull;
    double x = f64(du), y = f64(dd);
    unsigned long long dsub = bits64(x * y - 1.0);
    unsigned long long dfused = bits64(fma(x, y, -1.0));
    if (dsub || dfused != 0xb970000000000000ull) return 3;
    printf("double sub=%016llx fma=%016llx\n", dsub, dfused);

    volatile unsigned integer = 16777217u;
    volatile double halfway = 1.000000059604644775390625;
    unsigned converted = bits32((float)integer);
    unsigned narrowed = bits32((float)halfway);
    unsigned divided = bits32((float)16777214u / 16777215.0f);
    if (converted != 0x4b800000u || narrowed != 0x3f800000u || divided != 0x3f7fffffu) return 4;
    printf("conversions integer=%08x narrow=%08x divide=%08x\n", converted, narrowed, divided);

    /* Portable annotations erase under Clang; fisiCs must retain the same
     * arithmetic while also validating the dimensions and concrete unit. */
    float distance FISICS_DIM(length) FISICS_UNIT(meter) = a;
    float offset FISICS_DIM(length) FISICS_UNIT(meter) = -1.0f;
    float result FISICS_DIM(length) FISICS_UNIT(meter) = distance * b + offset;
    if (bits32(result) != 0) return 5;
    printf("units result=%08x\n", bits32(result));
    return 0;
}
