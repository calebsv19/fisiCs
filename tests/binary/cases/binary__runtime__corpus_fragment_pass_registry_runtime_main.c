typedef struct {
    const char *name;
    unsigned weight;
} PassInfo;

unsigned registry_count(void);
unsigned registry_weight_sum(void);
const PassInfo *registry_pick(unsigned idx);

int printf(const char *fmt, ...);

int main(void) {
    const PassInfo *p = registry_pick(1u);
    unsigned w = p ? p->weight : 0u;
    printf("%u %u %u\n", registry_count(), registry_weight_sum(), w);
    return 0;
}
