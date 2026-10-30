#include "../../../include/fisics/extensions.h"
#ifndef FAIL_KIND
#define FAIL_KIND 0
#endif
#if FAIL_KIND == 4
#define RESULT FISICS_DIM(time)
#elif FAIL_KIND == 5
#define RESULT FISICS_DIM(length) FISICS_UNIT(foot)
#else
#define RESULT FISICS_DIM(length) FISICS_UNIT(meter)
#endif
/* The helper always promises meters; contradictory contracts are rejected
 * by the ordinary cross-TU ledger before linking. */
#ifdef RETURN_HELPER
#undef RESULT
#define RESULT FISICS_DIM(length) FISICS_UNIT(meter)
#endif
double RESULT travel(double speed FISICS_DIM(speed),double dt FISICS_DIM(time));
