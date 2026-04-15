// Regression: typedef-tag equivalent function pointer parameter types should
// be assignment-compatible.
struct U;
typedef struct U U;
typedef struct Meta {
    int value;
} Meta;

struct Pane {
    void (*handler)(struct U*, Meta);
};

void handle(U* u, Meta m) {
    (void)u;
    (void)m;
}

void bind(struct Pane* pane) {
    pane->handler = handle;
}
