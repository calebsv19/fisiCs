// SPDX-License-Identifier: Apache-2.0

extern unsigned long edu08_sink(unsigned long value);

unsigned long edu08_leaf(unsigned long seed) {
    volatile unsigned long scratch[20];
    scratch[0] = seed;
    scratch[19] = seed ^ 0x5a5aUL;
    return scratch[0] + scratch[19];
}

unsigned long edu08_probe(unsigned long seed) {
    return edu08_sink(edu08_leaf(seed));
}
