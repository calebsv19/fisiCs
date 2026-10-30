/* P3.3 strict-policy controls, shared C source; not a runtime workload. */
#include <fisics/extensions.h>
#define D(x) FISICS_DIM(x)
extern double external_force(void) D(force);
double literal_force(void) D(force) { double value D(force)=2; return value; }
double external_only(void) D(force) { return external_force(); }
void counter_only(void) { for(int i=0;i<2;i++) {} }
void scalar_body(double force D(force)) { int count=1; force=force; count=count+1; }
void counter_enters_physics(double force D(force)) {
    for(int i=0;i<2;i++) { force=force*i; }
}
void annotated_counter(double force D(force)) {
    for(int i D(time)=0;i<i;i++) { force=force; }
}
void raw_bound(double force D(force),int count) {
    for(int i=0;i<count;i++) { force=force; }
}
void physical_integer(int value D(time),double force D(force)) { value=value; force=force; }
void ordinary_scale(double force D(force),double scale) { force=force*scale; }
void explicit_ratio(double force D(force),double scale D(1)) { force=force*scale; }
void aggregate_unknown(double force D(force)) { double values[2]={1,2};force=force; }
void wrong_dimension(double force D(force),double dt D(time)) {
#ifdef BAD_DIM
    force=dt;
#else
    force=force;
#endif
}
