#ifndef BINARY_CORPUS_FRAGMENT_JOB_GRAPH_H
#define BINARY_CORPUS_FRAGMENT_JOB_GRAPH_H

typedef struct {
    unsigned id;
    unsigned deps[4];
    unsigned dep_count;
} Job;

unsigned job_weight(const Job *j);
int job_is_ready(const Job *j, const unsigned *done, unsigned done_count);
unsigned graph_ready_score(const Job *jobs, unsigned n, const unsigned *done, unsigned done_count);

#endif
