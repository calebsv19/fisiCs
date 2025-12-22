#include <time.h>

int main(void) {
    struct tm tm = {0};
    time_t now = time(NULL);
    (void)tm;
    (void)now;
    return 0;
}
