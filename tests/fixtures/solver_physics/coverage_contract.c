/* P3.1 input: no runtime execution; deliberately uninitialized raw state. */
#include <fisics/extensions.h>
extern double external_force(void) FISICS_DIM(force);
extern double external_unknown(void);
struct Raw { double acceleration; };
struct Typed { double acceleration FISICS_DIM(acceleration); };
void coverage_sample(void) {
 double force FISICS_DIM(force)=2, moment FISICS_DIM(force*length)=3;
 double meters FISICS_DIM(length) FISICS_UNIT(meter)=2,
        feet FISICS_DIM(length) FISICS_UNIT(foot)=3;
 double distance FISICS_DIM(length)=2;
 double scalar FISICS_DIM(1)=1, raw;
 double read_force=force, read_moment=moment, read_feet=feet, read_raw=raw;
 double sum=force+force;
#ifdef BAD_COVERAGE_ANNOTATION
 double malformed FISICS_DIM(length) FISICS_UNIT(second)=1;
 double observed_malformed=malformed;
#endif
 double external_value=external_force(), unknown_value=external_unknown();
 struct Raw body; struct Typed typed;
 double matrix[6][6];
 double raw_field=body.acceleration, raw_slot=matrix[0][0], typed_field=typed.acceleration;
 (void)distance;(void)scalar;(void)read_force;(void)read_moment;(void)read_feet;(void)read_raw;
 (void)sum;(void)external_value;(void)unknown_value;(void)raw_field;(void)raw_slot;(void)typed_field;
}
