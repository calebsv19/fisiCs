/* Contained source qualification only. Raw SI restoration is separately trusted.
 * Momentum is the scalar normal momentum transfer; no vector/frame inference. */
#ifndef P714_CONTACT_ENERGY_H
#define P714_CONTACT_ENERGY_H
#ifdef __FISICS__
#include <fisics/extensions.h>
#define CE_D(x) FISICS_DIM(x)
#ifdef CONTACT_PHYSICAL
#define CE_K(x) [[fisics::quantity_kind(x)]]
#else
#define CE_K(x)
#endif
#else
#define CE_D(x)
#define CE_K(x)
#endif
#define CE_E CE_D(energy) CE_K("quantity.energy")
#define CE_V CE_D(length/time) CE_K("quantity.velocity")
#define CE_M CE_D(mass) CE_K("quantity.mass")
#define CE_P CE_D(mass*length/time) CE_K("quantity.momentum")
#define CE_R CE_D(1) CE_K("quantity.ratio")
static inline double CE_E ce_kinetic(double m CE_M,double v CE_V){double half CE_R=.5;return half*m*v*v;}
static inline double CE_P ce_momentum_delta(double m CE_M,double now CE_V,double old CE_V){return m*(now-old);}
static inline double CE_P ce_impulse(double closing CE_V,double target CE_V,double effective CE_M){return (closing-target)*effective;}
static inline double CE_E ce_loss(double before CE_E,double after CE_E){return before-after;}
static inline double CE_E ce_balance(double after CE_E,double before CE_E,double loss CE_E){return after-before+loss;}
/* Sphere rotational term is dimensional only; zero spin is the selected profile.
 * Do not claim a canonical rotational form from mass/radius raw restoration. */
static inline double CE_D(energy) ce_spin(double m CE_D(mass),double r CE_D(length),double w CE_D(1/time)){return .2*m*r*r*w*w;}
#endif
