#ifndef AXIS12_STATIC_INLINE_LOCAL_STATE_SHARED_H
#define AXIS12_STATIC_INLINE_LOCAL_STATE_SHARED_H

static inline unsigned axis12_tick(unsigned delta) {
    static unsigned counter = 0u;
    counter = counter * 9u + delta;
    return counter;
}

unsigned axis12_worker(unsigned seed);

#endif
