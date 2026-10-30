#include <stdio.h>
#ifdef USE_TGMATH
#include <tgmath.h>
#else
#include <math.h>
#endif
#include "../../../include/fisics/extensions.h"
#ifdef __FISICS__
#define METERS(x) fisics_convert_unit((x),"meter")
#else
#define METERS(x) ((x)*0.3048)
#endif
#ifndef FAIL_KIND
#define FAIL_KIND 0
#endif
static void takes_time(double t FISICS_DIM(time)){(void)t;}
#if FAIL_KIND == 9
static double FISICS_DIM(time) wrong(double x FISICS_DIM(length)) {return fabs(x);}
#endif
int main(void){
    double x FISICS_DIM(length) FISICS_UNIT(foot)=3;
    double y FISICS_DIM(length) FISICS_UNIT(foot)=4;
    double t FISICS_DIM(time)=2;
    double m FISICS_DIM(length) FISICS_UNIT(meter)=1;
#if FAIL_KIND == 1
    double result FISICS_DIM(time)=fabs(x);(void)result;
#elif FAIL_KIND == 2
    (void)hypot(x,t);
#elif FAIL_KIND == 3
    (void)fmax(x,t);
#elif FAIL_KIND == 4
    (void)hypot(x,m);
#elif FAIL_KIND == 5
    (void)fmax(x,m);
#elif FAIL_KIND == 6
    (void)fmax(x,0);
#elif FAIL_KIND == 7
    (void)hypot(0,x);
#elif FAIL_KIND == 8
    takes_time(hypot(x,y));
#elif FAIL_KIND == 9
    (void)wrong(x);
#elif FAIL_KIND == 10
    double result FISICS_DIM(length) FISICS_UNIT(meter)=fmax(x,y);(void)result;
#elif FAIL_KIND == 11
    double unknown_unit FISICS_DIM(length)=4;
    (void)METERS(hypot(x,unknown_unit));
#else
    /* Reduced solver patterns: displacement norm, magnitude, and a bounded maximum. */
    double zero FISICS_DIM(length) FISICS_UNIT(foot)=0;
    double norm FISICS_DIM(length) FISICS_UNIT(foot)=hypot(x,y);
    double magnitude FISICS_DIM(length) FISICS_UNIT(foot)=fabs(-x);
    double maximum FISICS_DIM(length) FISICS_UNIT(foot)=fmax(magnitude,zero);
    double nested FISICS_DIM(length) FISICS_UNIT(foot)=fmax(fabs(-x),hypot(x,y));
    double converted FISICS_DIM(length) FISICS_UNIT(meter)=hypot(METERS(x),METERS(y));
    printf("%.9f %.9f %.9f %.9f %.9f\n",METERS(norm),METERS(magnitude),METERS(maximum),METERS(nested),converted);
#endif
    return 0;
}
