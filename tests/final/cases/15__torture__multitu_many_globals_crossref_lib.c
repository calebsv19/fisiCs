int main_seed(void);
int main_bias_at(int idx);

int g_lib_a = 3;
int g_lib_b = 5;
int g_lib_c = 11;
int g_lib_d = 13;
int g_lib_e = 17;
int g_lib_f = 19;
int g_lib_table[8] = {1, 4, 9, 16, 25, 36, 49, 64};
int g_lib_skew[6] = {6, 1, 8, 0, 3, 7};

int lib_mix(int idx) {
    int left = g_lib_table[idx & 7] + g_lib_skew[idx % 6];
    int right = main_bias_at(idx) + main_seed();
    int scale = (g_lib_a + g_lib_b + g_lib_c + g_lib_d + g_lib_e + g_lib_f) % 23;
    return left * 2 + right + scale + idx;
}

int lib_window_sum(int span) {
    int sum = 0;
    for (int i = 0; i < span; ++i) {
        int base = main_bias_at(i) + main_seed();
        sum += base + g_lib_table[(i + span) & 7];
    }
    return sum;
}
