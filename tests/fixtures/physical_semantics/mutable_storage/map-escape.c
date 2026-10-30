#include <fisics/extensions.h>
#define E FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define Q FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
void sink(const unsigned*); E double check(double a E){const unsigned map[1]={0};double s[2];s[map[0]]=a;sink(map);return s[0];}
