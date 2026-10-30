// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <fisics/extensions.h>
#ifndef FAIL_KIND
#define FAIL_KIND 0
#endif
struct Loads { double force FISICS_DIM(force), moment FISICS_DIM(force*length); };
double global_force FISICS_DIM(force)=120, global_moment FISICS_DIM(force*length)=200;
int main(void) {
 double force FISICS_DIM(force)=120, moment FISICS_DIM(force*length)=200;
 double meters FISICS_DIM(length) FISICS_UNIT(meter)=2,
        feet FISICS_DIM(length) FISICS_UNIT(foot)=3;
 double f[2] FISICS_DIM(force)={10,20}, m[2] FISICS_DIM(force*length)={30,40};
 double *fp FISICS_DIM(force)=&force, *mp FISICS_DIM(force*length)=&moment;
 struct Loads loads={120,200};
#if FAIL_KIND == 1
 force=moment;
#elif FAIL_KIND == 2
 global_force=global_moment;
#elif FAIL_KIND == 3
 meters=feet;
#elif FAIL_KIND == 4
 f[1]=m[1];
#elif FAIL_KIND == 5
 *fp=*mp;
#elif FAIL_KIND == 6
 loads.force=loads.moment;
#elif FAIL_KIND == 7
 for(double a FISICS_DIM(force)=120,b FISICS_DIM(force*length)=200;0;){a=b;}
#elif FAIL_KIND == 8
 double a FISICS_DIM(force)=force,b FISICS_DIM(force*length)=a;
#elif FAIL_KIND == 9
 (void)(force+moment);
#else
 double torque FISICS_DIM(force*length)=moment;
 double copied_feet FISICS_DIM(length) FISICS_UNIT(foot)=feet;
 for(double a FISICS_DIM(force)=1,b FISICS_DIM(force*length)=2;a<force;a+=force) {
   double rotation_work FISICS_DIM(force*length)=b;
   torque+=rotation_work;
 }
 printf("%.1f %.1f %.1f %.1f %.1f %.1f %.1f\n",force,torque,copied_feet,m[1],*mp,loads.moment,global_moment);
#endif
 (void)meters;(void)feet;(void)fp;(void)mp;(void)f;(void)m;(void)loads;
 return 0;
}
