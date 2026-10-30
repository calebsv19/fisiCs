/* Coincident local 2D elastic joint fixture. Mask bits: x=1,y=2,rotation=4.
 * Source convention: force_on_a = stiffness*(q_b-q_a). Kq is the conjugate
 * external nodal load required to hold the displacement, hence -force_on_a at A.
 * Finite nonnegative spring coefficients are caller-admitted SI values. */
#ifndef FISICS_TEST_FRAME_JOINT_H
#define FISICS_TEST_FRAME_JOINT_H
#include "frame_loading.h"
typedef struct FrameJointStiffness {
    double translation[2] FISICS_DIM(force/length);
    double rotation FISICS_DIM(force*length);
} FrameJointStiffness;
typedef struct FrameJointResponse {
    double relative_translation[2] FISICS_DIM(length);
    double relative_rotation FISICS_DIM(scalar);
    double force_on_a[2] FISICS_DIM(force);
    double moment_on_a FISICS_DIM(force*length);
    double energy FISICS_DIM(energy);
} FrameJointResponse;
static void frame_joint_build(const FrameJointStiffness *s,unsigned mask,FrameLocalStiffness *k){
    double zero_tt FISICS_DIM(force/length)=0;
    double zero_coupling FISICS_DIM(force)=0;
    double zero_rr FISICS_DIM(force*length)=0;
    for(unsigned i=0;i<4;++i){for(unsigned j=0;j<4;++j)k->tt[i][j]=zero_tt;
        for(unsigned j=0;j<2;++j)k->tr[i][j]=zero_coupling;}
    for(unsigned i=0;i<2;++i){for(unsigned j=0;j<4;++j)k->rt[i][j]=zero_coupling;
        for(unsigned j=0;j<2;++j)k->rr[i][j]=zero_rr;}
    for(unsigned a=0;a<2;++a)if(mask&(1u<<a)){
        k->tt[a][a]=s->translation[a];k->tt[a+2][a+2]=s->translation[a];
        k->tt[a][a+2]=-s->translation[a];k->tt[a+2][a]=-s->translation[a];}
    if(mask&4){k->rr[0][0]=s->rotation;k->rr[1][1]=s->rotation;k->rr[0][1]=-s->rotation;k->rr[1][0]=-s->rotation;}
}
static void frame_joint_response(const FrameJointStiffness *s,unsigned mask,
    const FrameLocalCoordinates *q,FrameJointResponse *out){
    double zero_force FISICS_DIM(force)=0;
    double zero_moment FISICS_DIM(force*length)=0;
    double zero_energy FISICS_DIM(energy)=0;
    out->energy=zero_energy;
    for(unsigned a=0;a<2;++a){
        out->relative_translation[a]=q->translation[a+2]-q->translation[a];
        out->force_on_a[a]=zero_force;
        if(mask&(1u<<a))out->force_on_a[a]=s->translation[a]*out->relative_translation[a];
        out->energy+=.5*out->force_on_a[a]*out->relative_translation[a];
    }
    out->relative_rotation=q->rotation[1]-q->rotation[0];out->moment_on_a=zero_moment;
    if(mask&4)out->moment_on_a=s->rotation*out->relative_rotation;
    out->energy+=.5*out->moment_on_a*out->relative_rotation;
}
#endif
