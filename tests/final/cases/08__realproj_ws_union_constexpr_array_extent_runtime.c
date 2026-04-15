// Regression: real-project Stage E blocker family (workspace_sandbox)
// Constant-expression array extents in union members must lower as arrays (not pointers).

#include <stdio.h>
#include <string.h>

#define WS_CFG_MAX_STRING_LENGTH 95

typedef struct WsConfigValue {
    int type;
    union {
        _Bool as_bool;
        long long as_int;
        double as_double;
        char as_string[WS_CFG_MAX_STRING_LENGTH + 1];
    } data;
} WsConfigValue;

int main(void) {
    WsConfigValue value = {0};
    value.type = 4;
    strcpy(value.data.as_string, "hello");
    printf("%zu %zu %zu %d\n",
           sizeof(WsConfigValue),
           sizeof(value.data),
           strlen(value.data.as_string),
           (int)value.data.as_string[0]);
    return 0;
}
