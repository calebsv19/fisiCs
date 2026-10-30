/* Source-shaped typed load fixture; finite SI inputs and admitted geometry.
 * No global assembly, model admission, or raw metadata inference. */
#ifndef FISICS_TEST_FRAME_LOADING_H
#define FISICS_TEST_FRAME_LOADING_H
#include "frame_boundary.h"

typedef struct FrameLineLoad { double component[2] FISICS_DIM(force/length); } FrameLineLoad;
typedef struct FrameLoadInput {
    double density FISICS_DIM(mass/length/length/length);
    double gravity[2] FISICS_DIM(acceleration);
    FrameLineLoad global, local;
} FrameLoadInput;
typedef struct FrameReleaseRotations { double value[2] FISICS_DIM(scalar); } FrameReleaseRotations;

static void frame_uniform_local(const FrameLocalSection *s,const FrameLocalBasis *basis,
    const FrameLoadInput *input,FrameLineLoad *out){
    double mass_per_length FISICS_DIM(mass/length)=input->density*s->area;
    double gx FISICS_DIM(force/length)=mass_per_length*input->gravity[0]+input->global.component[0];
    double gy FISICS_DIM(force/length)=mass_per_length*input->gravity[1]+input->global.component[1];
    out->component[0]=basis->cosine*gx+basis->sine*gy+input->local.component[0];
    out->component[1]=-basis->sine*gx+basis->cosine*gy+input->local.component[1];
}
static void frame_equivalent_load(const FrameLocalSection *s,const FrameLineLoad *q,FrameLocalLoads *out){
    out->force[0]=q->component[0]*s->length/2;
    out->force[2]=out->force[0];
    out->force[1]=q->component[1]*s->length/2;
    out->force[3]=out->force[1];
    out->moment[0]=q->component[1]*s->length*s->length/12;
    out->moment[1]=-out->moment[0];
}
static void frame_release_rotations(const FrameLocalStiffness *k,const FrameLocalLoads *rhs,
    bool release_a,bool release_b,FrameReleaseRotations *out){
    double zero FISICS_DIM(scalar)=0;out->value[0]=zero;out->value[1]=zero;
    if(release_a && release_b){
        double ratio FISICS_DIM(scalar)=k->rr[1][0]/k->rr[0][0];
        out->value[1]=(rhs->moment[1]-ratio*rhs->moment[0])/(k->rr[1][1]-ratio*k->rr[0][1]);
        out->value[0]=(rhs->moment[0]-k->rr[0][1]*out->value[1])/k->rr[0][0];
    }else if(release_a)out->value[0]=rhs->moment[0]/k->rr[0][0];
    else if(release_b)out->value[1]=rhs->moment[1]/k->rr[1][1];
}
static void frame_condensed_load(const FrameLocalStiffness *k,const FrameLocalLoads *input,
    bool release_a,bool release_b,FrameLocalLoads *out){
    FrameLocalLoads original=*input;FrameReleaseRotations rotation;
    frame_release_rotations(k,&original,release_a,release_b,&rotation);
    for(unsigned i=0;i<4;++i)out->force[i]=original.force[i]-k->tr[i][0]*rotation.value[0]-k->tr[i][1]*rotation.value[1];
    for(unsigned i=0;i<2;++i)out->moment[i]=original.moment[i]-k->rr[i][0]*rotation.value[0]-k->rr[i][1]*rotation.value[1];
    double zero FISICS_DIM(force*length)=0;
    if(release_a)out->moment[0]=zero;
    if(release_b)out->moment[1]=zero;
}
static void frame_add_loads(const FrameLocalLoads *a,const FrameLocalLoads *b,FrameLocalLoads *out){
    FrameLocalLoads left=*a,right=*b;
    for(unsigned i=0;i<4;++i)out->force[i]=left.force[i]+right.force[i];
    for(unsigned i=0;i<2;++i)out->moment[i]=left.moment[i]+right.moment[i];
}
static double FISICS_DIM(energy) frame_nodal_work(const FrameLocalCoordinates *q,const FrameLocalLoads *p){
    double work FISICS_DIM(energy)=0;
    for(unsigned i=0;i<4;++i)work+=q->translation[i]*p->force[i];
    for(unsigned i=0;i<2;++i)work+=q->rotation[i]*p->moment[i];
    return work;
}
#endif
