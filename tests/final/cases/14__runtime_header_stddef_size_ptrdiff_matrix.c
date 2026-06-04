#include <stddef.h>
#include <stdio.h>

struct Wave22StddefStrideNode {
    int lane;
    char tag;
};

int main(void) {
    struct Wave22StddefStrideNode nodes[6];
    ptrdiff_t elem_delta = &nodes[5] - &nodes[1];
    ptrdiff_t byte_delta = (char *)&nodes[5] - (char *)&nodes[1];
    size_t node_size = sizeof(nodes[0]);
    size_t total_size = sizeof(nodes);
    void *null_marker = NULL;
    unsigned long score = (unsigned long)(elem_delta + byte_delta + (ptrdiff_t)node_size + (ptrdiff_t)total_size);

    printf("stddef-size-ptrdiff elem=%ld byte=%ld node=%lu total=%lu null=%d score=%lu\n",
           (long)elem_delta,
           (long)byte_delta,
           (unsigned long)node_size,
           (unsigned long)total_size,
           null_marker == NULL,
           score);
    return score == 92ul ? 0 : 1;
}
