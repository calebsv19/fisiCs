#include <stdbool.h>

typedef struct {
    int value;
} Payload;

bool invalid_state(void) {
    Payload payload = {1};
    return payload;
}
