const unsigned axis1_wave3_table[4] = {11u, 7u, 5u, 3u};

unsigned axis1_wave3_fetch(int index) {
    return axis1_wave3_table[(unsigned)index & 3u];
}
