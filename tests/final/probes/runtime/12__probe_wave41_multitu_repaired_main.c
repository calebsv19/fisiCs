#line 14181 "virtual_wave41_multitu_repaired_main.c"
int printf(const char *format, ...);
int wave41_worker(int value);

int main(void) {
    int result = wave41_worker(38);
    printf("%d\n", result);
    return result == 41 ? 0 : 1;
}
