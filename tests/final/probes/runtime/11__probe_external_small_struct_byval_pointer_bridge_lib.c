#include <stdio.h>

typedef struct ProbeColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} ProbeColor;

typedef struct ProbeRect {
    int x;
    int y;
    int w;
    int h;
} ProbeRect;

int probe_external_small_struct_bridge(const char *text, ProbeColor color, ProbeRect *rect) {
    printf("text=%s color=%u,%u,%u,%u rect_ok=%d\n",
           text,
           (unsigned)color.r,
           (unsigned)color.g,
           (unsigned)color.b,
           (unsigned)color.a,
           rect ? 1 : 0);
    if (!rect) {
        return 11;
    }
    rect->x += (int)color.r;
    return rect->x;
}
