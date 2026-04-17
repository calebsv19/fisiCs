typedef unsigned char Uint8;

typedef union {
    Uint8 padding[sizeof(void *) <= 8 ? 56 : sizeof(void *) == 16 ? 64 : 3 * sizeof(void *)];
} Event;

_Static_assert(sizeof(Event) == sizeof(((Event *)0)->padding), "event padding size mismatch");

int main(void) {
    return 0;
}
