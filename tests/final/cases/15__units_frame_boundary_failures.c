#include "../../fixtures/solver_physics/frame_boundary.h"
#ifndef FAIL_KIND
#define FAIL_KIND 1
#endif
#if FAIL_KIND == 3
static void take_force(double x FISICS_DIM(force)){(void)x;}
#endif
#if FAIL_KIND == 4
static double FISICS_DIM(force) wrong(FrameLocalLoads *p){return p->moment[0];}
#endif
int main(void){
    FrameLocalStiffness k;FrameLocalBasis b;FrameLocalCoordinates q;FrameLocalLoads p;
#if FAIL_KIND == 1
    b.cosine=q.translation[0];
#elif FAIL_KIND == 2
    double moment FISICS_DIM(force*length)=k.tt[0][0];(void)moment;
#elif FAIL_KIND == 3
    take_force(k.rr[0][0]);
#elif FAIL_KIND == 4
    (void)wrong(&p);
#elif FAIL_KIND == 5
    q.translation[0]=q.rotation[0];
#elif FAIL_KIND == 6
    p.moment[0]=p.force[0];
#elif FAIL_KIND == 7
    double ft FISICS_DIM(length) FISICS_UNIT(foot)=1;q.translation[0]=ft;
#elif FAIL_KIND == 8
    FrameDense6 d;frame_dense_restore_assumed_si(&d,&k);k.tt[0][0]=k.rr[0][0];
#elif FAIL_KIND == 9
    k.tt[0][0]-=k.rr[0][0];
#endif
    (void)k;(void)b;(void)q;(void)p;return 0;
}
