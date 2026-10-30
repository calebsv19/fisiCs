#include <stdio.h>
#if defined(USE_TGMATH)
#include <tgmath.h>
#else
#include <math.h>
#endif
#include "../../../include/fisics/extensions.h"
#ifdef __FISICS__
#define ROOT_METERS(value, scale) fisics_convert_unit((value),"meter")
#else
#define ROOT_METERS(value, scale) ((value)*(scale))
#endif
#ifndef FAIL_KIND
#define FAIL_KIND 0
#endif
#if FAIL_KIND == 7
static double FISICS_DIM(time) wrong_return(double v FISICS_DIM(speed)) {
    return sqrt(v*v);
}
#endif
static void takes_time(double dt FISICS_DIM(time)) {(void)dt;}
int main(void) {
    double vx FISICS_DIM(speed) FISICS_UNIT(meter_per_second)=3;
    double vy FISICS_DIM(speed) FISICS_UNIT(meter_per_second)=4;
#if FAIL_KIND == 1
    double result FISICS_DIM(time)=sqrt(vx*vx+vy*vy);
    (void)result;
#elif FAIL_KIND == 2
    double length FISICS_DIM(length) FISICS_UNIT(meter)=4;
    (void)sqrt(length);
#elif FAIL_KIND == 3
    double inverse_time FISICS_DIM(s^-1)=4;
    (void)sqrt(inverse_time);
#elif FAIL_KIND == 4
    float area FISICS_DIM(area)=4;
    float result FISICS_DIM(time)=sqrtf(area);
    (void)result;
#elif FAIL_KIND == 5
    long double area FISICS_DIM(area)=4;
    long double result FISICS_DIM(time)=sqrtl(area);
    (void)result;
#elif FAIL_KIND == 6
    takes_time(sqrt(vx*vx+vy*vy));
#elif FAIL_KIND == 7
    (void)wrong_return(vx);
#elif FAIL_KIND == 8
    double area FISICS_DIM(area) FISICS_UNIT(square_centimeter)=4;
    double result FISICS_DIM(length) FISICS_UNIT(meter)=sqrt(area);
    (void)result;
#else
    double norm FISICS_DIM(speed)=sqrt(vx*vx+vy*vy);
    double area_m FISICS_DIM(area) FISICS_UNIT(square_meter)=4;
    double area_mm FISICS_DIM(area) FISICS_UNIT(square_millimeter)=4;
    double area_cm FISICS_DIM(area) FISICS_UNIT(square_centimeter)=4;
    double fourth_m FISICS_DIM(m^4) FISICS_UNIT(meter_to_fourth)=16;
    double fourth_mm FISICS_DIM(m^4) FISICS_UNIT(millimeter_to_fourth)=16;
    double fourth_cm FISICS_DIM(m^4) FISICS_UNIT(centimeter_to_fourth)=16;
    printf("%.9f %.9f %.9f %.9f %.9f %.9f %.9f\n",norm,
        ROOT_METERS(sqrt(area_m),1.0),
        ROOT_METERS(sqrt(area_mm),0.001),
        ROOT_METERS(sqrt(area_cm),0.01),
        ROOT_METERS(sqrt(sqrt(fourth_m)),1.0),
        ROOT_METERS(sqrt(sqrt(fourth_mm)),0.001),
        ROOT_METERS(sqrt(sqrt(fourth_cm)),0.01));
#endif
    return 0;
}
