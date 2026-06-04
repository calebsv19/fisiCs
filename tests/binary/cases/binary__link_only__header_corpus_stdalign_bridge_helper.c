#include <stdalign.h>
#include <stddef.h>

struct Wave15BridgePayload {
    alignas(16) unsigned char bytes[16];
    int tail;
};

int header_corpus_wave15_stdalign_bridge_summary(void) {
    return (int)alignof(struct Wave15BridgePayload) * 100 +
           (int)offsetof(struct Wave15BridgePayload, bytes) * 10 +
           (int)offsetof(struct Wave15BridgePayload, tail);
}
