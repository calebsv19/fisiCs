typedef struct LargePayload {
    unsigned char bytes[131072];
    int marker;
} LargePayload;

LargePayload make_large_payload(int seed) {
    LargePayload value = {0};
    value.bytes[0] = (unsigned char)seed;
    value.bytes[131071] = (unsigned char)(seed + 1);
    value.marker = seed + 2;
    return value;
}
