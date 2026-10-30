#include "../../fixtures/solver_physics/frame_local.h"
#ifndef FAIL_KIND
#define FAIL_KIND 1
#endif
#if FAIL_KIND == 7
static void take_time(double t FISICS_DIM(time)){(void)t;}
#endif
#if FAIL_KIND == 8
static double FISICS_DIM(force) wrong_result(FrameLocalLoads *p){return p->moment[0];}
#endif
int main(void){
    FrameLocalStiffness k;FrameLocalCoordinates q;FrameLocalLoads p;
#if FAIL_KIND == 1
    k.tt[0][0]=k.rr[0][0];
#elif FAIL_KIND == 2
    k.tr[1][0]=k.tt[1][1];
#elif FAIL_KIND == 3
    k.rt[0][1]=k.rr[0][0];
#elif FAIL_KIND == 4
    k.rr[0][0]=k.tt[0][0];
#elif FAIL_KIND == 5
    p.force[0]=p.moment[0];
#elif FAIL_KIND == 6
    q.translation[0]=q.rotation[0];
#elif FAIL_KIND == 7
    take_time(p.force[0]);
#elif FAIL_KIND == 8
    (void)wrong_result(&p);
#elif FAIL_KIND == 9
    p.force[0]+=k.tt[0][0]*q.rotation[0];
#elif FAIL_KIND == 10
    p.moment[0]+=k.rr[0][0]*q.translation[0];
#elif FAIL_KIND == 11
    double ft FISICS_DIM(length) FISICS_UNIT(foot)=1;
    q.translation[0]=ft;
#endif
    (void)k;(void)q;(void)p;
    return 0;
}
