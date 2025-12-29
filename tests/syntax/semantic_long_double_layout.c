// Validate long double and pointer-sized typedefs follow TargetLayout defaults (LP64+fp80).
#include <stddef.h>

int check_ld_sz[sizeof(long double) == 16 ? 1 : -1];
int check_ld_align[_Alignof(long double) == 16 ? 1 : -1];
int check_max_align[_Alignof(max_align_t) == 16 ? 1 : -1];
int check_size_t[sizeof(size_t) == 8 ? 1 : -1];
int check_ptrdiff_t[sizeof(ptrdiff_t) == 8 ? 1 : -1];

int main(void) {
    long double ld = 1.0L;
    return (int)(ld + check_ld_sz[0] + check_ld_align[0] +
                 check_max_align[0] + check_size_t[0] + check_ptrdiff_t[0]);
}
