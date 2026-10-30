#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
#define V FISICS_DIM(length/time) FISICS_QUANTITY_KIND("quantity.velocity")
#define W FISICS_DIM(1/time) FISICS_QUANTITY_KIND("quantity.angular_rate")
#define M FISICS_DIM(mass) FISICS_QUANTITY_KIND("quantity.mass")
E double check(double a E,double b Q){const double s[2]={a,b};const double t[2]={b,a};return s[0]+t[0];}
