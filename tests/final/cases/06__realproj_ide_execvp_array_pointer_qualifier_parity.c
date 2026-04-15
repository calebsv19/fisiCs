// Regression: function call parameter compatibility must treat
// `char *const argv[]` and `char * const *` consistently.
#include <unistd.h>

int run_exec(char* const argv[]) {
    return execvp(argv[0], argv);
}
