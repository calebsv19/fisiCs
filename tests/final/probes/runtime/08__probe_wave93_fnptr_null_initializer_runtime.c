struct Opaque;

typedef int (*OpenHandler)(const char *, int, int, char *, unsigned long, void *);
typedef int (*EditHandler)(const char *, char *, unsigned long, void *, struct Opaque **);

static OpenHandler g_open = (void *)0;

int main(void) {
    OpenHandler open = (void *)0;
    EditHandler edit = (void *)0;
    return g_open == open && edit == (void *)0 ? 0 : 1;
}
