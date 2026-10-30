#include <fisics/extensions.h>
#if defined(COSMIC_PHYSICAL)
#define CP(d,k) FISICS_DIM(d) FISICS_QUANTITY_KIND(k)
#elif defined(COSMIC_UNITS)
#define CP(d,k) FISICS_DIM(d)
#else
#define CP(d,k)
#endif
static inline double CP(energy,"quantity.energy") cp_kinetic_2d(double m CP(mass,"quantity.mass"),double vx CP(speed,"quantity.velocity"),double vy CP(speed,"quantity.velocity")) {
 double half CP(1,"quantity.ratio")=.5;
 return half*m*(vx*vx+vy*vy);
}
static inline double CP(length^3/time^2,"quantity.gravitational_parameter") cp_mu(double g CP(length^3/mass/time^2,"quantity.gravitational_constant"),double mass CP(mass,"quantity.mass")){return g*mass;}
static inline double CP(force,"quantity.force") cp_force(double mu CP(length^3/time^2,"quantity.gravitational_parameter"),double mass CP(mass,"quantity.mass"),double r2 CP(length^2,"quantity.area")){return mu*mass/r2;}
static inline double CP(acceleration,"quantity.acceleration") cp_acceleration(double f CP(force,"quantity.force"),double m CP(mass,"quantity.mass")){return f/m;}
static inline double CP(energy,"quantity.energy") cp_pair_potential(double mu CP(length^3/time^2,"quantity.gravitational_parameter"),double mass CP(mass,"quantity.mass"),double r CP(length,"quantity.geometric_length")){return -mu*mass/r;}
