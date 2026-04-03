#include "binary__corpus_fragment_job_graph.h"

unsigned job_weight(const Job *j) {
    return j->id + j->dep_count;
}

unsigned graph_ready_score(const Job *jobs, unsigned n, const unsigned *done, unsigned done_count) {
    unsigned i = 0;
    unsigned score = 0;
    while (i < n) {
        if (job_is_ready(&jobs[i], done, done_count)) {
            score += job_weight(&jobs[i]);
        }
        ++i;
    }
    return score;
}
