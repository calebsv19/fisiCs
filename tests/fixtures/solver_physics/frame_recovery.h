/* Typed local member recovery for finite admitted SI inputs. Normalized xi
 * keeps polynomial coefficients homogeneous. No global solve, generic polynomial
 * type erasure, strength qualification, or source-wide checking claim. */
#ifndef FISICS_TEST_FRAME_RECOVERY_H
#define FISICS_TEST_FRAME_RECOVERY_H
#include "frame_loading.h"
#include <math.h>
typedef struct FrameMemberFields {
    double axial_displacement[3] FISICS_DIM(length);
    double transverse_displacement[5] FISICS_DIM(length);
    double axial[2] FISICS_DIM(force);
    double moment[3] FISICS_DIM(force*length);
    double shear[2] FISICS_DIM(force);
    double tension FISICS_DIM(pressure),compression FISICS_DIM(pressure),max_shear FISICS_DIM(pressure);
    double energy FISICS_DIM(energy),distributed_work FISICS_DIM(energy);
} FrameMemberFields;
static void frame_recover_coordinates(const FrameLocalStiffness *k,const FrameLocalLoads *load,
    const FrameLocalCoordinates *retained,bool release_a,bool release_b,FrameLocalCoordinates *out){
    FrameLocalCoordinates q=*retained;double zero FISICS_DIM(scalar)=0;
    if(release_a)q.rotation[0]=zero;
    if(release_b)q.rotation[1]=zero;
    FrameLocalLoads elastic,rhs;frame_local_apply(k,&q,&elastic);
    for(unsigned i=0;i<4;++i)rhs.force[i]=load->force[i]-elastic.force[i];
    for(unsigned i=0;i<2;++i)rhs.moment[i]=load->moment[i]-elastic.moment[i];
    FrameReleaseRotations rotation;frame_release_rotations(k,&rhs,release_a,release_b,&rotation);
    if(release_a)q.rotation[0]=rotation.value[0];
    if(release_b)q.rotation[1]=rotation.value[1];
    *out=q;
}
static void frame_recover_end(const FrameLocalStiffness *k,const FrameLocalCoordinates *q,
    const FrameLocalLoads *equivalent,FrameLocalLoads *out){
    FrameLocalLoads elastic,load=*equivalent;frame_local_apply(k,q,&elastic);
    for(unsigned i=0;i<4;++i)out->force[i]=elastic.force[i]-load.force[i];
    for(unsigned i=0;i<2;++i)out->moment[i]=elastic.moment[i]-load.moment[i];
}
static void frame_stress_envelope(FrameMemberFields *f,double area FISICS_DIM(area),
    double modulus FISICS_DIM(length*length*length)){
    double zero_pressure FISICS_DIM(pressure)=0;
    double zero FISICS_DIM(scalar)=0,one FISICS_DIM(scalar)=1;
    f->tension=zero_pressure;f->compression=zero_pressure;
    for(int side=-1;side<=1;side+=2){
        double sign FISICS_DIM(scalar)=(double)side;
        double p[3] FISICS_DIM(pressure);
        p[0]=f->axial[0]/area+sign*f->moment[0]/modulus;
        p[1]=f->axial[1]/area+sign*f->moment[1]/modulus;
        p[2]=sign*f->moment[2]/modulus;
        double xi[3] FISICS_DIM(scalar)={0,1,0};unsigned count=2;
        if(p[2]!=zero_pressure){double interior FISICS_DIM(scalar)=-p[1]/(2*p[2]);
            if(interior>zero && interior<one)xi[count++]=interior;}
        for(unsigned i=0;i<count;++i){
            double stress FISICS_DIM(pressure)=p[0]+xi[i]*(p[1]+xi[i]*p[2]);
            f->tension=fmax(f->tension,stress);f->compression=fmax(f->compression,-stress);}
    }
    f->max_shear=1.5*fmax(fabs(f->shear[0]),fabs(f->shear[0]+f->shear[1]))/area;
}
static void frame_recover_fields(const FrameLocalSection *s,double modulus FISICS_DIM(length*length*length),
    const FrameLineLoad *line,const FrameLocalCoordinates *q,FrameMemberFields *f){
    double ea FISICS_DIM(force)=s->young*s->area;
    double ei FISICS_DIM(force*length*length)=s->young*s->inertia;
    double axial_bubble FISICS_DIM(length)=line->component[0]*s->length*s->length/(2*ea);
    double bubble FISICS_DIM(length)=line->component[1]*s->length*s->length*s->length*s->length/(24*ei);
    f->axial_displacement[0]=q->translation[0];
    f->axial_displacement[1]=q->translation[2]-q->translation[0]+axial_bubble;
    f->axial_displacement[2]=-axial_bubble;
    double dv FISICS_DIM(length)=q->translation[3]-q->translation[1]-q->rotation[0]*s->length-bubble;
    double dr FISICS_DIM(length)=(q->rotation[1]-q->rotation[0])*s->length-4*bubble;
    f->transverse_displacement[0]=q->translation[1];f->transverse_displacement[1]=q->rotation[0]*s->length;
    f->transverse_displacement[2]=3*dv-dr;f->transverse_displacement[3]=dr-2*dv;f->transverse_displacement[4]=bubble;
    f->axial[0]=ea/s->length*f->axial_displacement[1];f->axial[1]=ea/s->length*2*f->axial_displacement[2];
    f->moment[0]=ei/s->length/s->length*2*f->transverse_displacement[2];
    f->moment[1]=ei/s->length/s->length*6*f->transverse_displacement[3];
    f->moment[2]=ei/s->length/s->length*12*f->transverse_displacement[4];
    f->shear[0]=ei/s->length/s->length/s->length*6*f->transverse_displacement[3];
    f->shear[1]=ei/s->length/s->length/s->length*24*f->transverse_displacement[4];
    double axial_integral FISICS_DIM(force)=0,moment_integral FISICS_DIM(force)=0,work_integral FISICS_DIM(force)=0;
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j){
        double power FISICS_DIM(scalar)=(double)(i+j+1);axial_integral+=f->axial[i]*f->axial[j]/(ea*power);}
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
        double power FISICS_DIM(scalar)=(double)(i+j+1);moment_integral+=f->moment[i]*f->moment[j]/(ei*power);}
    for(unsigned i=0;i<3;++i){double power FISICS_DIM(scalar)=(double)(i+1);
        work_integral+=line->component[0]*f->axial_displacement[i]/power;}
    for(unsigned i=0;i<5;++i){double power FISICS_DIM(scalar)=(double)(i+1);
        work_integral+=line->component[1]*f->transverse_displacement[i]/power;}
    f->energy=s->length*(axial_integral+moment_integral)/2;
    f->distributed_work=s->length*work_integral;
    frame_stress_envelope(f,s->area,modulus);
}
#endif
