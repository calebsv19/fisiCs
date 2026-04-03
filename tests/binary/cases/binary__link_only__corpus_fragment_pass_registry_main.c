typedef struct {
    const char *name;
    unsigned weight;
} PassInfo;

unsigned registry_count(void);
unsigned registry_weight_sum(void);
const PassInfo *registry_pick(unsigned idx);

int main(void) {
    const PassInfo *p = registry_pick(1u);
    return (int)(registry_count() + registry_weight_sum() + (p ? p->weight : 0u));
}
