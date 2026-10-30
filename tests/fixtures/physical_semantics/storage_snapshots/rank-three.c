#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
#define V FISICS_DIM(length/time) FISICS_QUANTITY_KIND("quantity.velocity")
#define W FISICS_DIM(1/time) FISICS_QUANTITY_KIND("quantity.angular_rate")
#define M FISICS_DIM(mass) FISICS_QUANTITY_KIND("quantity.mass")
E double check(double a E){const double s[1][1][1]={{{a}}};return s[0][0][0];}
