/* Reduced from orbit_body_integrate: raw stored state, annotated local bridge,
 * semi-implicit Euler ordering, pointer access and component arrays retained. */
#include <fisics/extensions.h>
#include <stdio.h>
#ifdef TYPED_STATE
typedef struct {
    double position[2] FISICS_DIM(length);
    double velocity[2] FISICS_DIM(speed);
    double acceleration[2] FISICS_DIM(acceleration);
} Body;
#else
typedef struct { double position[2], velocity[2], acceleration[2]; } Body;
#endif
static void integrate(Body *body, double seconds) {
    double dt FISICS_DIM(time) FISICS_UNIT(second) = seconds;
    double velocity FISICS_DIM(speed) = body->velocity[0];
    double position FISICS_DIM(length) = body->position[0];
#ifdef BAD_RAW_FIELD
    body->acceleration[0] = dt; /* desired: reject time stored as acceleration */
#endif
    double acceleration FISICS_DIM(acceleration) = body->acceleration[0];
    velocity = velocity + acceleration * dt;
#ifdef BAD_UPDATE
    position = position + acceleration; /* desired: reject */
#else
    position = position + velocity * dt;
#endif
    body->velocity[0] = velocity;
    body->position[0] = position;
}
int main(void) {
    Body body = {{0,0},{1,0},{3,0}};
    integrate(&body,0.5);
    printf("%.9f %.9f\n",body.position[0],body.velocity[0]);
    return 0;
}
