#include <stdarg.h>

struct pair48 {
    int a;
    int b;
};

struct big48 {
    int lanes[5];
    long total;
};

typedef int (*pair48_cb)(struct pair48 item, int salt);

struct big48 wave48_variadic_callback_sret_bridge(int seed, pair48_cb cb, int count, ...) {
    struct big48 out;
    va_list ap;
    long total = seed * 3 + count;
    int i;

    for (i = 0; i < 5; i++) {
        out.lanes[i] = seed + i;
    }

    va_start(ap, count);
    for (i = 0; i < count && i < 5; i++) {
        int value = va_arg(ap, int);
        struct pair48 item;
        int routed;
        item.a = value;
        item.b = seed + i;
        routed = cb(item, i + 1);
        out.lanes[i] = routed + value;
        total += out.lanes[i] * (i + 2);
    }
    va_end(ap);

    for (; i < 5; i++) {
        total += out.lanes[i];
    }

    out.total = total;
    return out;
}
