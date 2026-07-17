struct wave77_packet {
    double x;
    double y;
    long bias;
};

struct wave77_result {
    long a;
    long b;
    long c;
    long d;
};

typedef struct wave77_result (*wave77_callback_fn)(struct wave77_packet packet,
                                                    int salt,
                                                    long bias);

static struct wave77_result wave77_left(struct wave77_packet packet,
                                        int salt,
                                        long bias) {
    struct wave77_result out;
    out.a = (long)(packet.x * 10.0) + salt;
    out.b = (long)(packet.y * 10.0) + bias;
    out.c = packet.bias + salt + bias;
    out.d = out.a + out.b + out.c;
    return out;
}

static struct wave77_result wave77_right(struct wave77_packet packet,
                                         int salt,
                                         long bias) {
    struct wave77_result out;
    out.a = (long)(packet.x * 8.0) - salt;
    out.b = (long)(packet.y * 4.0) + bias;
    out.c = packet.bias * 2 + salt - bias;
    out.d = out.a - out.b + out.c;
    return out;
}

wave77_callback_fn wave77_factory(int mode) {
    return (mode & 1) == 0 ? wave77_left : wave77_right;
}
