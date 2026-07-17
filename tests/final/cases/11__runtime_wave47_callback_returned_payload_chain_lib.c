typedef struct Seed47C {
    int base;
    int delta;
} Seed47C;

typedef struct Payload47C {
    int values[3];
    int tag;
} Payload47C;

typedef struct Result47C {
    Payload47C payloads[2];
    int marker;
} Result47C;

typedef Payload47C (*PayloadCallback47C)(Seed47C, int);

Result47C wave47_callback_returned_payload_chain(PayloadCallback47C callback, Seed47C seed, int route) {
    Result47C result;
    result.payloads[0] = callback(seed, route);
    seed.base += 2;
    seed.delta += 1;
    result.payloads[1] = callback(seed, route + 2);
    result.marker = result.payloads[0].tag + result.payloads[1].tag + route;
    return result;
}
