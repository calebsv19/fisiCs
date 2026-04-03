#include "binary__corpus_fragment_job_graph.h"

int printf(const char *fmt, ...);

int main(void) {
    const Job jobs[2] = {
        {1u, {0u, 0u, 0u, 0u}, 0u},
        {2u, {1u, 0u, 0u, 0u}, 1u}
    };
    const unsigned done[1] = {1u};
    unsigned score = graph_ready_score(jobs, 2u, done, 1u);
    printf("%u\n", score);
    return 0;
}
