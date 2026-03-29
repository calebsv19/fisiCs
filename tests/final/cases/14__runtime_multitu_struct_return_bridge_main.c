#include <stdio.h>

struct ABIRecord {
    int id;
    long long total;
    double weight;
};

struct ABIRecord abi_make_record(int base, long long extra);
long long abi_fold_record(struct ABIRecord rec);

int main(void) {
    struct ABIRecord rec = abi_make_record(7, 9000000000LL);
    long long folded = abi_fold_record(rec);
    int weight_ok = (rec.weight > 3.4) && (rec.weight < 3.6);

    printf("%d %lld %d\n", rec.id, folded, weight_ok);
    return 0;
}

