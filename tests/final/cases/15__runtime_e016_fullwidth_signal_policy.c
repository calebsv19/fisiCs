/* Compiler-owned reduction of ESP_Work's NormalizedSignalPolicy.c.
 * Source SHA-256 at intake: b0160f5f2f64f83aff501b851727130a2ee114f314e4f1ca2341db81fb6cb0d3.
 * Checks C99 host arithmetic, narrowing, wrap and aggregate output only.
 * This snapshot is not the production portable-C source or firmware evidence. */
#include <stdint.h>
#include <stddef.h>
extern int printf(const char *, ...);

enum result {
    ok = 0, invalid_output = 1, invalid_maximum = 2, out_of_range = 3,
    invalid_published_flag = 4, invalid_change_threshold = 5,
    invalid_periodic_interval = 6
};
enum reason { initial = 0, change = 1, periodic = 2, hold = 3 };
struct decision {
    uint8_t should_publish;
    enum reason why;
    uint16_t movement;
    uint32_t elapsed_ms;
};

static enum result normalize(uint16_t filtered, uint16_t maximum, uint16_t *out) {
    uint32_t numerator;
    if (out == NULL) return invalid_output;
    if (maximum == 0U) return invalid_maximum;
    if (filtered > maximum) return out_of_range;
    numerator = (uint32_t)filtered * 1000U + (uint32_t)(maximum / 2U);
    *out = (uint16_t)(numerator / (uint32_t)maximum);
    return ok;
}

static enum result decide(uint8_t has_published, uint16_t current,
                          uint16_t last, uint16_t threshold, uint32_t now,
                          uint32_t last_ms, uint32_t interval,
                          struct decision *out) {
    struct decision value;
    if (out == NULL) return invalid_output;
    if (has_published > 1U) return invalid_published_flag;
    if (threshold == 0U) return invalid_change_threshold;
    if (interval == 0U) return invalid_periodic_interval;
    if (has_published == 0U) {
        value.should_publish = 1U;
        value.why = initial;
        value.movement = 0U;
        value.elapsed_ms = 0U;
        *out = value;
        return ok;
    }
    value.movement = current >= last ? (uint16_t)(current - last)
                                     : (uint16_t)(last - current);
    value.elapsed_ms = (uint32_t)(now - last_ms);
    if (value.movement >= threshold) {
        value.should_publish = 1U;
        value.why = change;
    } else if (value.elapsed_ms >= interval) {
        value.should_publish = 1U;
        value.why = periodic;
    } else {
        value.should_publish = 0U;
        value.why = hold;
    }
    *out = value;
    return ok;
}

int main(void) {
    static const struct { uint16_t filtered, maximum, expected; } adc[] = {
        {0U, 65535U, 0U}, {1U, 65535U, 0U},
        {32767U, 65535U, 500U}, {32768U, 65535U, 500U},
        {65534U, 65535U, 1000U}, {65535U, 65535U, 1000U},
        {1U, 7U, 143U}, {3U, 7U, 429U}
    };
    uint16_t output = 77U;
    struct decision publication;
    unsigned int row;
    for (row = 0U; row < sizeof(adc) / sizeof(adc[0]); row = row + 1U) {
        if (normalize(adc[row].filtered, adc[row].maximum, &output) != ok ||
            output != adc[row].expected) return 1;
    }
    output = 77U;
    if (normalize(1U, 0U, &output) != invalid_maximum || output != 77U ||
        normalize(2U, 1U, &output) != out_of_range || output != 77U ||
        normalize(1U, 1U, NULL) != invalid_output) return 2;
    if (decide(1U, 0U, 65535U, 65535U, 0U, UINT32_MAX, 2U, &publication) != ok ||
        publication.why != change || publication.movement != 65535U ||
        publication.elapsed_ms != 1U || publication.should_publish != 1U) return 3;
    if (decide(1U, 0U, 0U, 1U, 0U, UINT32_MAX, 1U, &publication) != ok ||
        publication.why != periodic || publication.elapsed_ms != 1U ||
        publication.should_publish != 1U) return 4;
    if (decide(1U, 0U, 0U, 1U, 0U, UINT32_MAX, 2U, &publication) != ok ||
        publication.why != hold || publication.elapsed_ms != 1U ||
        publication.should_publish != 0U) return 5;
    if (decide(0U, 65535U, 0U, 1U, 0U, UINT32_MAX, 2U, &publication) != ok ||
        publication.why != initial || publication.movement != 0U ||
        publication.elapsed_ms != 0U || publication.should_publish != 1U) return 6;
    publication.should_publish = 9U;
    publication.why = hold;
    publication.movement = 88U;
    publication.elapsed_ms = 99U;
    if (decide(2U, 1U, 1U, 1U, 0U, 0U, 1U, &publication) != invalid_published_flag ||
        decide(1U, 1U, 1U, 0U, 0U, 0U, 1U, &publication) != invalid_change_threshold ||
        decide(1U, 1U, 1U, 1U, 0U, 0U, 0U, &publication) != invalid_periodic_interval ||
        decide(1U, 1U, 1U, 1U, 0U, 0U, 1U, NULL) != invalid_output ||
        publication.should_publish != 9U || publication.why != hold ||
        publication.movement != 88U || publication.elapsed_ms != 99U) return 7;
    printf("E016 fullwidth policy adc=8 invalid=3 publication=4 decision-invalid=4 result=PASS\n");
    return 0;
}
