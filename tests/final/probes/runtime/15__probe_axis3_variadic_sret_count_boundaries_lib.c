#include <stdarg.h>

struct CountPacket {
    long double value;
    unsigned tag;
};

struct CountPacket collect_counted(long double seed, unsigned tag, int count, ...) {
    struct CountPacket result;
    va_list args;
    int i;

    result.value = seed;
    result.tag = tag;
    va_start(args, count);
    for (i = 0; i < count; ++i) {
        if ((i & 1) == 0) {
            long double value = va_arg(args, long double);
            result.value += value * (long double)(i + 1) / 16.0L;
            result.tag = result.tag * 33u + (unsigned)(value * 16.0L);
        } else {
            int value = va_arg(args, int);
            result.value += (long double)value / 32.0L;
            result.tag = result.tag * 33u + (unsigned)value;
        }
    }
    va_end(args);
    return result;
}
