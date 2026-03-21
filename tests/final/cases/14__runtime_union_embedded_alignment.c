#include <stddef.h>
#include <stdio.h>

typedef union {
    long long ll;
    double d;
} Payload;

typedef struct {
    char c;
    Payload p;
    char end;
} Wrap;

int main(void) {
    Wrap w;
    w.c = 1;
    w.p.ll = 0x1122334455667788LL;
    w.end = 2;

    size_t off_payload = offsetof(Wrap, p);
    size_t align_like = offsetof(struct {
        char c;
        Payload p;
    }, p);

    int ok1 = (off_payload == align_like);
    int ok2 = (w.p.ll == 0x1122334455667788LL);

    w.p.d = 3.5;
    int ok3 = (w.p.d > 3.4) && (w.p.d < 3.6);
    int ok4 = (sizeof(Wrap) >= off_payload + sizeof(Payload));

    printf("%d %d %d %d\n", ok1, ok2, ok3, ok4);
    return 0;
}
