#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
#define V FISICS_DIM(length/time) FISICS_QUANTITY_KIND("quantity.velocity")
#define W FISICS_DIM(1/time) FISICS_QUANTITY_KIND("quantity.angular_rate")
#define M FISICS_DIM(mass) FISICS_QUANTITY_KIND("quantity.mass")
E double check(double a E){const double s[64]={a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a};return s[63];}
