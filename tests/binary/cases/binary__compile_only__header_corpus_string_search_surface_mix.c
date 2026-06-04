#include <stddef.h>
#include <string.h>

int wave23_string_search_surface(const char *text) {
    const char *dash = strchr(text, '-');
    const char *tail = strstr(text, "tail");
    size_t span = strcspn(text, ":");
    return dash != NULL && tail != NULL && span > 0;
}
