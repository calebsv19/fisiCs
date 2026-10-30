#include "../../fixtures/solver_physics/frame_recovery.h"
#include "../../fixtures/solver_physics/frame_joint.h"
#ifndef FAIL_KIND
#define FAIL_KIND 1
#endif
#if FAIL_KIND == 4
static void take_force(double x FISICS_DIM(force)){(void)x;}
#endif
#if FAIL_KIND == 5
static double FISICS_DIM(force) wrong(FrameMemberFields *f){return f->energy;}
#endif
int main(void){
    FrameLineLoad line;FrameLocalLoads p;FrameJointStiffness spring;FrameMemberFields f;
#if FAIL_KIND == 1
    line.component[0]=p.moment[0];
#elif FAIL_KIND == 2
    spring.translation[0]=spring.rotation;
#elif FAIL_KIND == 3
    double area FISICS_DIM(area)=1;frame_stress_envelope(&f,area,area);
#elif FAIL_KIND == 4
    take_force(f.energy);
#elif FAIL_KIND == 5
    (void)wrong(&f);
#elif FAIL_KIND == 6
    FrameLocalCoordinates q;double ft FISICS_DIM(length) FISICS_UNIT(foot)=1;q.translation[0]=ft;
#elif FAIL_KIND == 7
    FrameJointResponse response;response.energy=p.force[0];
#elif FAIL_KIND == 8
    double force FISICS_DIM(force)=p.moment[0];(void)force;
#elif FAIL_KIND == 9
    FrameLocalCoordinates q;double force FISICS_DIM(force)=frame_nodal_work(&q,&p);(void)force;
#endif
    (void)line;(void)p;(void)spring;(void)f;return 0;
}
