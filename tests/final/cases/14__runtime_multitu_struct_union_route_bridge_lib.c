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

Route make_route(int seed, int mode) {
    Route r;
    if (mode & 1) {
        r.payload.p.x = seed + mode;
        r.payload.p.y = seed * 2 - mode;
        r.tag = 1;
    } else {
        r.payload.ll = (long long)seed * 1003LL + (long long)mode * 17LL;
        r.tag = 0;
    }
    return r;
}

long long eval_route(Route r) {
    if (r.tag) {
        return (long long)r.payload.p.x * 11LL + (long long)r.payload.p.y * 13LL;
    }
    return r.payload.ll - 19LL;
}
