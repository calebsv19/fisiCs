#include <stdio.h>
#include <string.h>

typedef enum Wind {
    WIND_NONE,
    WIND_N,
    WIND_NE,
    WIND_E
} Wind;

typedef struct WindInfo {
    Wind preset;
    const char *id;
    const char *label;
    float vx;
    float vy;
} WindInfo;

static const float k_diag = 0.70710678f;

static const WindInfo k_presets[] = {
    {WIND_NONE, "none", "No Wind", 0.0f, 0.0f},
    {WIND_N, "n", "North", 0.0f, -1.0f},
    {WIND_NE, "ne", "North East", k_diag, -k_diag},
    {WIND_E, "e", "East", 1.0f, 0.0f}
};

int main(void) {
    unsigned i;
    unsigned score = 0;

    for (i = 0; i < sizeof(k_presets) / sizeof(k_presets[0]); ++i) {
        score += (unsigned)k_presets[i].preset;
        score += (unsigned)(k_presets[i].vx * 100.0f + 200.0f);
        score += (unsigned)(k_presets[i].vy * 100.0f + 200.0f);
        score += (unsigned)strlen(k_presets[i].id);
        score += (unsigned)strlen(k_presets[i].label);
    }

    printf("%u %s %.6f\n", score, k_presets[3].id, k_presets[2].vx);
    return strcmp(k_presets[3].id, "e") != 0;
}
