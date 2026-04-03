typedef struct {
    const char *name;
    unsigned weight;
} PassInfo;

unsigned registry_count(void);
const PassInfo *registry_pick(unsigned idx);

unsigned registry_weight_sum(void) {
    unsigned i = 0;
    unsigned acc = 0;
    unsigned n = registry_count();
    while (i < n) {
        const PassInfo *p = registry_pick(i);
        acc += p ? p->weight : 0u;
        ++i;
    }
    return acc;
}
