struct wave81_payload {
    long a;
    long b;
    long c;
};

struct wave81_result {
    long x;
    long y;
    long z;
    long w;
};

typedef long (*wave81_callback_fn)(struct wave81_payload payload, long salt);

struct wave81_bundle {
    wave81_callback_fn first;
    wave81_callback_fn second;
    long seed;
};

struct wave81_result wave81_dispatch(struct wave81_bundle bundle,
                                     struct wave81_payload payload) {
    struct wave81_result result;
    result.x = bundle.first(payload, bundle.seed);
    result.y = bundle.second(payload, bundle.seed + 1);
    result.z = result.x + result.y;
    result.w = result.x * 2 - result.y;
    return result;
}
