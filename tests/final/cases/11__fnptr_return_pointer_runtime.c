#include <string.h>

typedef const char *(*NameFn)(int value);

static const char *name_impl(int value) {
    (void)value;
    return "ALPHA";
}

int main(void) {
    NameFn fn = name_impl;
    const char *text = fn(7);
    if (!text) {
        return 1;
    }
    return strcmp(text, "ALPHA") == 0 ? 0 : 2;
}
