typedef struct Payload46C {
    int a;
    int b;
    int c;
} Payload46C;

typedef struct Small46C {
    unsigned long long x;
    unsigned long long y;
} Small46C;

typedef struct Large46C {
    unsigned long long slots[5];
} Large46C;

typedef Small46C (*Callback46C)(Payload46C, int);

Large46C wave46_large_sret_callback_payload(Callback46C callback, Payload46C payload, int route) {
    Small46C first = callback(payload, route);
    Payload46C next = {payload.a + 2, payload.b - 1, payload.c + 1};
    Small46C second = callback(next, route + 2);
    Large46C large;
    large.slots[0] = first.x;
    large.slots[1] = first.y;
    large.slots[2] = second.x;
    large.slots[3] = second.y;
    large.slots[4] = first.x + second.y;
    return large;
}
