#ifndef FISICS_13_WAVE65_MACRO_CASCADE_SUPPORT_H
#define FISICS_13_WAVE65_MACRO_CASCADE_SUPPORT_H

#line 6510 "virtual_wave65_macro_cascade_support.h"
#ifdef WAVE65_REPAIRED
#define WAVE65_LEAF wave65_present_operand
#else
#define WAVE65_LEAF wave65_missing_operand
#endif

#define WAVE65_INNER(value) ((value) + 5)
#define WAVE65_OUTER(value) WAVE65_INNER(value)
#define WAVE65_EXPRESSION() WAVE65_OUTER(WAVE65_LEAF)

#endif
