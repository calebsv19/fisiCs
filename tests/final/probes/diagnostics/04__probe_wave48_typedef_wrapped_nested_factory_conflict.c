typedef int (*wave48_conflict_leaf_old_t)();
typedef int (*wave48_conflict_leaf_int_t)(int);
typedef int (*wave48_conflict_leaf_double_t)(double);
typedef wave48_conflict_leaf_old_t (*wave48_conflict_factory_old_t)(wave48_conflict_leaf_old_t);
typedef wave48_conflict_leaf_int_t (*wave48_conflict_factory_int_t)(wave48_conflict_leaf_int_t);
typedef wave48_conflict_leaf_double_t (*wave48_conflict_factory_double_t)(wave48_conflict_leaf_double_t);

wave48_conflict_factory_old_t wave48_conflict_route();
wave48_conflict_factory_int_t wave48_conflict_route(void);
wave48_conflict_factory_double_t wave48_conflict_route(void);

int main(void) {
    return 0;
}
