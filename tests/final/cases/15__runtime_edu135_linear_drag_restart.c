/* Compiler-owned reduction of os-dev EDU-135's fixed h=tau=1 recurrence.
 * Source contract: docs/contracts/LINEAR_DRAG_PHYSICAL_PROFILE_V1.md.
 * This tests defined C arithmetic and restart-state passing, not the OS
 * artifact verifier, continuous analytic reference, or physical accuracy. */
extern int printf(const char *, ...);

struct drag_state {
    double position;
    double velocity;
};

static struct drag_state advance(struct drag_state state, double acceleration,
                                 unsigned int steps) {
    unsigned int step = 0U;
    while (step < steps) {
        state.velocity = (state.velocity + acceleration) * 0.5;
        state.position = state.position + state.velocity;
        step = step + 1U;
    }
    return state;
}

int main(void) {
    static const struct {
        struct drag_state start;
        double acceleration;
        struct drag_state expected_four;
    } vectors[] = {
        {{0.0, 0.0}, 2.0, {6.125, 1.875}},
        {{0.0, 0.0}, -2.0, {-6.125, -1.875}},
        {{3.0, -1.0}, 2.0, {8.1875, 1.8125}},
        {{-3.0, 8.0}, 0.0, {4.5, 0.5}}
    };
    unsigned int row;
    for (row = 0U; row < 4U; row = row + 1U) {
        struct drag_state direct = advance(vectors[row].start,
                                           vectors[row].acceleration, 4U);
        unsigned int split;
        if (direct.position != vectors[row].expected_four.position ||
            direct.velocity != vectors[row].expected_four.velocity) return 1;
        for (split = 0U; split <= 4U; split = split + 1U) {
            struct drag_state checkpoint = advance(vectors[row].start,
                                                   vectors[row].acceleration, split);
            struct drag_state restarted = advance(checkpoint,
                                                  vectors[row].acceleration, 4U - split);
            if (restarted.position != direct.position ||
                restarted.velocity != direct.velocity) return 2;
        }
    }
    printf("EDU135 drag recurrence cases=4 splits=20 result=PASS\n");
    return 0;
}
