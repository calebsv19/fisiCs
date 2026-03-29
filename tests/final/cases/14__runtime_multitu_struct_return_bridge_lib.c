struct ABIRecord {
    int id;
    long long total;
    double weight;
};

struct ABIRecord abi_make_record(int base, long long extra) {
    struct ABIRecord rec;
    rec.id = base + 2;
    rec.total = extra + (long long)base;
    rec.weight = 3.5;
    return rec;
}

long long abi_fold_record(struct ABIRecord rec) {
    return (long long)rec.id + rec.total + (long long)(rec.weight * 10.0);
}

