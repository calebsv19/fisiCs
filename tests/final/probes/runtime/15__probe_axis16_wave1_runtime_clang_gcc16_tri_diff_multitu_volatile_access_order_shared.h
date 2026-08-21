#ifndef AXIS16_VOLATILE_ACCESS_ORDER_SHARED_H
#define AXIS16_VOLATILE_ACCESS_ORDER_SHARED_H

void axis16_store(volatile unsigned *slot, unsigned value);
unsigned axis16_load(const volatile unsigned *slot);

#endif
