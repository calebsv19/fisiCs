#ifndef FISICS_PROBE_WAVE67_EXTERNAL_INLINE_SHARED_H
#define FISICS_PROBE_WAVE67_EXTERNAL_INLINE_SHARED_H

inline int wave67_external_adjust(int value) {
    return value * 5 + 3;
}

int wave67_external_from_provider(int value);
int wave67_external_from_observer(int value);

#endif
