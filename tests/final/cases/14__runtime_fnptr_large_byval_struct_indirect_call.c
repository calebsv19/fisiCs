#include <stdio.h>

typedef struct InputMeta {
    int cmd;
    int role;
    int x;
    int y;
    int mod;
    void* payload;
    void* target;
} InputMeta;

typedef struct Pane Pane;

struct Pane {
    void (*handleCommand)(Pane*, InputMeta);
    int sink;
};

static void handleCommandImpl(Pane* pane, InputMeta meta) {
    pane->sink = meta.cmd + meta.role + meta.x + meta.y + meta.mod +
                 (meta.payload != 0) + (meta.target != 0);
}

int main(void) {
    Pane pane = {0};
    pane.handleCommand = handleCommandImpl;

    InputMeta meta = {
        .cmd = 81,
        .role = 2,
        .x = 3,
        .y = 4,
        .mod = 5,
        .payload = (void*)0x1234,
        .target = 0
    };

    pane.handleCommand(&pane, meta);
    if (pane.sink != 96) {
        printf("bad:%d\n", pane.sink);
        return 1;
    }
    puts("ok");
    return 0;
}
