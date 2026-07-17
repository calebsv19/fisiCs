struct Wave129UnitsPayload {
    double distance_m[2];
    double reserve_j;
    double charge_ah;
};

typedef struct Wave129UnitsPayload (*Wave129UnitsCallback)(struct Wave129UnitsPayload, double);

struct Wave129UnitsPayload wave129_units_seed(double feet, double reserve_wh, double draw_mah);
struct Wave129UnitsPayload wave129_units_apply(struct Wave129UnitsPayload payload,
                                               Wave129UnitsCallback callback,
                                               double tick_ms);
double wave129_units_score(struct Wave129UnitsPayload payload, int lane);
