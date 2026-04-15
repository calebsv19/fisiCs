#include <stdio.h>

typedef struct VisualSelectionState {
    unsigned char payload_mask[8];
} VisualSelectionState;

static unsigned first_unvisited_masked(const VisualSelectionState *selection,
                                       unsigned char *visited,
                                       unsigned area) {
    unsigned idx;
    for (idx = 0u; idx < area; ++idx) {
        if (!selection->payload_mask[idx] || visited[idx]) {
            continue;
        }
        return idx;
    }
    return area;
}

int main(void) {
    VisualSelectionState selection = { {0u, 1u, 1u, 0u, 1u, 0u, 0u, 1u} };
    unsigned char visited[8] = {0u, 0u, 1u, 0u, 0u, 0u, 0u, 0u};
    unsigned idx = first_unvisited_masked(&selection, visited, 8u);
    printf("%u\n", idx);
    return 0;
}
