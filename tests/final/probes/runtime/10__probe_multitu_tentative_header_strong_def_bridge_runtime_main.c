#include <stdio.h>

#include "10__probe_multitu_tentative_header_strong_def_bridge_runtime.h"

int bucket10_runtime_width = 12;
int bucket10_runtime_height = 34;

int main(void) {
    printf("%d %d %d\n",
           bucket10_runtime_width,
           bucket10_runtime_height,
           bucket10_runtime_area());
    return 0;
}
