#include <stdio.h>

typedef union Payload {
    struct {
        int x;
        int y;
    } p;
    long long ll;
} Payload;

typedef struct Route {
    Payload payload;
    int tag;
} Route;

extern Route make_route(int seed, int mode);
extern long long eval_route(Route r);

int main(void) {
    long long acc = 0;
    for (int i = 0; i < 7; ++i) {
        Route r = make_route(10 + i * 3, i);
        acc = acc * 13LL + eval_route(r);
    }
    printf("%lld\n", acc);
    return 0;
}
