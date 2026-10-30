#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
#define V FISICS_DIM(length/time) FISICS_QUANTITY_KIND("quantity.velocity")
#define W FISICS_DIM(1/time) FISICS_QUANTITY_KIND("quantity.angular_rate")
#define M FISICS_DIM(mass) FISICS_QUANTITY_KIND("quantity.mass")
E double producer(void); E double check(double a E,double b Q){double known E=a;const double s[2]={producer(),b};return s[0];}
