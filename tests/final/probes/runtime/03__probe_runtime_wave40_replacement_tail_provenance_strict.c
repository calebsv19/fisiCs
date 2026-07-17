#include <stdio.h>

#define W40_PROVENANCE_STR_RAW(x) #x
#define W40_PROVENANCE_STR(x) W40_PROVENANCE_STR_RAW(x)
#define W40_PROVENANCE_RECORD(file, line) file ":" W40_PROVENANCE_STR(line)
#define W40_PROVENANCE_EMIT() 0, W40_PROVENANCE_RECORD

#line 4040 "virtual_wave40_tail_provenance.c"
int main(void) {
    printf("%s\n", (W40_PROVENANCE_EMIT()(__FILE__, __LINE__)));
    return 0;
}
