#include <fisics/extensions.h>
#ifdef PILOT_PHYSICAL
#define K(x) FISICS_QUANTITY_KIND(x)
#define O(x) FISICS_PHYSICAL_ORIGIN(x)
#else
#define K(x)
#define O(x)
#endif
#ifdef PILOT_UNITS
#define D(x) FISICS_DIM(x)
#else
#define D(x)
#endif
#define T D(temperature) K("quantity.temperature.absolute") O("origin.kelvin.zero")
#define DT D(temperature) K("quantity.temperature.difference")
#define A D(1/temperature) K("quantity.thermal_expansion_coefficient")
#define E D(1) K("quantity.mechanical_strain")
double E check(double t T,double ref T,double alpha A){ return alpha*(t-ref); }
int main(void){double a=check(350,300,12e-6),b=check(450,400,12e-6);if(a!=b||check(300,300,12e-6)!=0||check(350,300,0)!=0||check(350,300,-12e-6)!=-a)return 1;return 0;}
