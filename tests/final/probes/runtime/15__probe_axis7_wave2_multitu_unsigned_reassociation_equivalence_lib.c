static unsigned int weighted(const unsigned int *values, unsigned int index) {
    return values[index] * (index * 3u + 5u);
}

unsigned int grouped_fold(const unsigned int *values, unsigned int count) {
    unsigned int total = 0u;
    unsigned int i = 0u;
    for (; i + 1u < count; i += 2u) total += weighted(values, i) + weighted(values, i + 1u);
    if (i < count) total += weighted(values, i);
    return total;
}

unsigned int streamed_fold(const unsigned int *values, unsigned int count) {
    unsigned int total = 0u;
    for (unsigned int i = 0u; i < count; ++i) total += weighted(values, i);
    return total;
}
