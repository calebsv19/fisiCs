#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef bool (*ProgressCallback)(int value, void *user_data);

static bool callback_impl(int value, void *user_data) {
    return value == 7 && user_data == NULL;
}

static ProgressCallback choose_callback(bool enabled, ProgressCallback callback) {
    return enabled ? callback : NULL;
}

static bool callback_present(ProgressCallback callback) {
    return callback != NULL;
}

int main(void) {
    ProgressCallback selected = choose_callback(true, callback_impl);
    int first = callback_present(selected) && selected(7, NULL);
    selected = choose_callback(false, callback_impl);
    int second = !callback_present(selected);
    printf("fnptr_null_expr first=%d second=%d\n", first, second);
    return (first && second) ? 0 : 1;
}
