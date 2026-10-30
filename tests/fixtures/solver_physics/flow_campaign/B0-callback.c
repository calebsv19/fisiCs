#include <fisics/extensions.h>
#define D(x) FISICS_DIM(x)
#define Q(x) FISICS_QUANTITY_KIND(x)
#define E D(energy) Q("quantity.energy")
#define T D(energy) Q("quantity.torque")
#define A D(1) Q("quantity.angle")
#define R D(1) Q("quantity.ratio")
#define P D(length) Q("quantity.position") FISICS_PHYSICAL_ORIGIN("origin.world")
#define L D(length) Q("quantity.displacement")
static int helper(int x){return x+1;}
int main(void){return helper(3)!=4;}
