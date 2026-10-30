#include <fisics/extensions.h>
#include <stdbool.h>
#define D(x) FISICS_DIM(x)
struct Storage { double x D(length) FISICS_UNIT(meter); double time D(time); };
typedef struct Storage Storage;
static void commit(Storage *p) {
#ifdef BAD_HELPER
    p->x=p->time;
#else
    p->x=p->x;
#endif
}
extern void external_commit(Storage *p);
extern double raw_value(void);
static void scalar_commit(double *p) { *p=*p; }
void caller(Storage *p) { p->x=p->x;commit(p); }
void external_caller(Storage *p) { p->x=p->x;external_commit(p); }
void scalar_caller(double *p,double x D(length)) { x=x;scalar_commit(p); }
void literals(Storage *p,double ratio D(1)) {
    p->x=2;p->x=-2;p->x=(double)3;p->x=1+2;p->x=p->x*ratio;
}
void incompatible(Storage *p,double ratio D(1)) {
#ifdef BAD_VARIABLE
    p->x=ratio;
#elif defined(BAD_COMPOUND)
    p->x+=2;
#elif defined(BAD_TIME)
    p->x=p->time;
#elif defined(BAD_CALL)
    p->x=raw_value();
#else
    p->x=p->x;
#endif
}
void unit_boundary(Storage *p,double cm D(length) FISICS_UNIT(centimeter)) {
#ifdef BAD_UNIT
    p->x=cm;
#else
    p->x=p->x;
#endif
}
bool guards(Storage *p,double x D(length)) {
    if(!p || (x>x && p)) {x=x;return false;}
    x=x;return true;
}
bool physical_guard(double x D(length)) {if(!x)x=x;return true;}
bool physical_status(Storage *p) {p->x=p->x;return p->x;}
