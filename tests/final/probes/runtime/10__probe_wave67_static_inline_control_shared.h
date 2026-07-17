#ifndef FISICS_PROBE_WAVE67_STATIC_INLINE_CONTROL_SHARED_H
#define FISICS_PROBE_WAVE67_STATIC_INLINE_CONTROL_SHARED_H

static inline int wave67_static_adjust(int value) {
    return value * 7 - 1;
}

int wave67_static_from_lib(int value);

#endif
