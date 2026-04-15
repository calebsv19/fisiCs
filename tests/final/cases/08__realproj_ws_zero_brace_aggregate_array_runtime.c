// Regression: real-project Stage A blocker family (workspace_sandbox)
// Aggregate arrays with `{0}` must zero-initialize correctly.

#include <stdio.h>

typedef struct WsCornerOverride {
    int qx;
    int qy;
    unsigned flags;
} WsCornerOverride;

typedef struct WsNodeRecord {
    unsigned node_index;
    unsigned node_id;
    float ratio;
} WsNodeRecord;

static int sum_corner_overrides(void) {
    WsCornerOverride corner_overrides[8] = {0};
    int sum = 0;
    int i = 0;
    for (i = 0; i < 8; ++i) {
        sum += corner_overrides[i].qx;
        sum += corner_overrides[i].qy;
        sum += (int)corner_overrides[i].flags;
    }
    return sum;
}

static int sum_node_records(void) {
    WsNodeRecord nodes[4] = {0};
    int sum = 0;
    int i = 0;
    for (i = 0; i < 4; ++i) {
        sum += (int)nodes[i].node_index;
        sum += (int)nodes[i].node_id;
        sum += (int)nodes[i].ratio;
    }
    return sum;
}

int main(void) {
    printf("%d %d\n", sum_corner_overrides(), sum_node_records());
    return 0;
}
