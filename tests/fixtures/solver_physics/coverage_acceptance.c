/* Final P3 acceptance mutations. Shared C; not an application solver. */
#include <fisics/extensions.h>
#include "frame_local.h"
#ifdef OMIT_DT
#define DT
#else
#define DT FISICS_DIM(time)
#endif
#ifdef OMIT_SCALE
#define SCALE
#else
#define SCALE FISICS_DIM(1)
#endif
#ifdef OMIT_POSITION
#define POSITION
#else
#define POSITION FISICS_DIM(length)
#endif
#ifdef OMIT_VELOCITY
#define VELOCITY
#else
#define VELOCITY FISICS_DIM(speed)
#endif
#ifdef OMIT_ACCELERATION
#define ACCELERATION
#else
#define ACCELERATION FISICS_DIM(acceleration)
#endif
#ifdef OMIT_DELTA
#define DELTA
#else
#define DELTA FISICS_DIM(speed)
#endif
#ifdef OMIT_NEXT
#define NEXT
#else
#define NEXT FISICS_DIM(speed)
#endif
#ifdef OMIT_RETURN
#define RESULT
#else
#define RESULT FISICS_DIM(length)
#endif
struct AcceptanceState {
    double position POSITION;
    double velocity VELOCITY;
    double acceleration ACCELERATION;
};
double acceptance_advance(struct AcceptanceState* state,double dt DT,double scale SCALE) RESULT {
    double delta DELTA=state->acceleration*dt;
    double next NEXT=state->velocity+delta;
    for(int i=0;i<2;i++) { next=next*scale; }
    state->velocity=next;
    state->position=state->position+next*dt;
    return state->position;
}
/* These boundaries do not alter the valid path's metadata or arithmetic. */
extern double opaque_length(void);
void acceptance_boundary(double length FISICS_DIM(length)) {
    length=length;
#ifdef OPAQUE_BOUNDARY
    length=opaque_length();
#endif
#ifdef SIZEOF_BOUNDARY
    (void)sizeof(int);
#endif
#ifdef AGGREGATE_BOUNDARY
    double values[2]={1,2};
    (void)values;
#endif
}
void acceptance_units(double a FISICS_DIM(length) FISICS_UNIT(meter),
                      double b FISICS_DIM(length) FISICS_UNIT(centimeter)) {
#ifdef WRONG_UNIT
    a=b;
#else
    a=a;
#endif
}
