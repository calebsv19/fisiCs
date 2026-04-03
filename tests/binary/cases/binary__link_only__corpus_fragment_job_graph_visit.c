#include "binary__corpus_fragment_job_graph.h"

static int contains(const unsigned *items, unsigned n, unsigned v) {
    unsigned i = 0;
    while (i < n) {
        if (items[i] == v) return 1;
        ++i;
    }
    return 0;
}

int job_is_ready(const Job *j, const unsigned *done, unsigned done_count) {
    unsigned i = 0;
    while (i < j->dep_count) {
        if (!contains(done, done_count, j->deps[i])) return 0;
        ++i;
    }
    return 1;
}
