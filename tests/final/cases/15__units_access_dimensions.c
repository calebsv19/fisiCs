#include <stdio.h>
#include "../../../include/fisics/extensions.h"
#ifndef FAIL_KIND
#define FAIL_KIND 0
#endif
typedef struct {
    double position[2] FISICS_DIM(length);
    double velocity[2] FISICS_DIM(speed);
    double acceleration[2] FISICS_DIM(acceleration);
    double length[2] FISICS_DIM(length) FISICS_UNIT(meter);
} Body;
static void takes_time(double dt FISICS_DIM(time)) {(void)dt;}
#if FAIL_KIND == 4
static double FISICS_DIM(time) wrong_return(Body *body) {return body->acceleration[0];}
#endif
static void integrate(Body *body) {
    double dt FISICS_DIM(time) FISICS_UNIT(second)=0.5;
#if FAIL_KIND == 1
    body->acceleration[0]=dt;
#elif FAIL_KIND == 2
    double duration FISICS_DIM(time)=body->acceleration[0];
    (void)duration;
#elif FAIL_KIND == 3
    takes_time(body->acceleration[0]);
#elif FAIL_KIND == 4
    (void)wrong_return(body);
#elif FAIL_KIND == 5
    body->acceleration[0]+=dt;
#elif FAIL_KIND == 6
    double ft FISICS_DIM(length) FISICS_UNIT(foot)=3;
    body->length[0]=ft;
#elif FAIL_KIND == 7
    double ft FISICS_DIM(length) FISICS_UNIT(foot)=body->length[0];
    (void)ft;
#endif
    double velocity_delta FISICS_DIM(speed)=body->acceleration[0]*dt;
    body->velocity[0]+=velocity_delta;
    double displacement FISICS_DIM(length)=body->velocity[0]*dt;
    body->position[0]+=displacement;
}
int main(void) {
    Body body={{0,0},{1,0},{3,0},{0,0}};
    integrate(&body);
    printf("%.6f %.6f\n",body.velocity[0],body.position[0]);
    return 0;
}
