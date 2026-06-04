#include <stddef.h>

struct Wave22StddefBridgeNode {
    char tag;
    int value;
    double weight;
    char tail;
};

size_t wave22_stddef_bridge_layout_score(void) {
    return offsetof(struct Wave22StddefBridgeNode, value) +
           offsetof(struct Wave22StddefBridgeNode, weight) +
           offsetof(struct Wave22StddefBridgeNode, tail) +
           sizeof(struct Wave22StddefBridgeNode);
}
