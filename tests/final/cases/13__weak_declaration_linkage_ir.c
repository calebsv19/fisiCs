extern int wave60_weak_data __attribute__((weak));
extern int wave60_weak_transform(int) __attribute__((weak));

extern int wave60_data_weak_after;
extern int wave60_data_weak_after __attribute__((weak));
extern int wave60_data_weak_before __attribute__((weak));
extern int wave60_data_weak_before;

extern int wave60_function_weak_after(int);
extern int wave60_function_weak_after(int) __attribute__((weak));
extern int wave60_function_weak_before(int) __attribute__((weak));
extern int wave60_function_weak_before(int);

int *wave60_data_address(void) {
    return &wave60_weak_data;
}

int (*wave60_function_address(void))(int) {
    return wave60_weak_transform;
}

int wave60_redeclaration_linkage_sum(void) {
    return (&wave60_data_weak_after != (int *)0) +
           (&wave60_data_weak_before != (int *)0) +
           (wave60_function_weak_after != (int (*)(int))0) +
           (wave60_function_weak_before != (int (*)(int))0);
}
