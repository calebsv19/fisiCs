extern int printf(const char*, ...);

typedef int wave22_values6_t[6];

extern int wave22_multitu_typedef_array_fold(wave22_values6_t values);

int main(void) {
    wave22_values6_t values = {1, 3, 5, 7, 9, 11};
    printf("%d\n", wave22_multitu_typedef_array_fold(values));
    return 0;
}
