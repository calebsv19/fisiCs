/* Source-shaped release/transform oracle and boundary qualification only. */
#include "../../fixtures/solver_physics/frame_boundary.h"
#include <math.h>
#include <stdio.h>
static void reference_local(const FrameLocalSection *s, double out[6][6]) {
    double L=s->length, a=s->young*s->area/L;
    double b=s->young*s->inertia/(L*L*L), l2=L*L;
    double matrix[6][6] = {
        {a,0,0,-a,0,0},
        {0,12*b,6*b*L,0,-12*b,6*b*L},
        {0,6*b*L,4*b*l2,0,-6*b*L,2*b*l2},
        {-a,0,0,a,0,0},
        {0,-12*b,-6*b*L,0,12*b,-6*b*L},
        {0,6*b*L,2*b*l2,0,-6*b*L,4*b*l2}
    };
    for (unsigned i=0;i<6;++i) for (unsigned j=0;j<6;++j) out[i][j]=matrix[i][j];
}
static double observe_coefficient(const FrameLocalStiffness *k,unsigned i,unsigned j) {
    unsigned ti=i<3?i:i-1, tj=j<3?j:j-1;
    if (i%3==2) {
        if (j%3==2) return k->rr[i/3][j/3];
        return k->rt[i/3][tj];
    }
    if (j%3==2) return k->tr[ti][j/3];
    return k->tt[ti][tj];
}
static int close_value(double a,double b,double scale) {
    return isfinite(a)&&isfinite(b)&&isfinite(scale)&&fabs(a-b)<=1e-9+1e-10*scale;
}
static void sample(unsigned n,const FrameLocalSection *s,const FrameLocalBasis *basis,FrameLocalCoordinates *q) {
    double zero_length FISICS_DIM(length) FISICS_UNIT(meter)=0;
    double zero_rotation FISICS_DIM(scalar)=0;
    double unit_length FISICS_DIM(length) FISICS_UNIT(meter)=1;
    double axial_translation FISICS_DIM(length) FISICS_UNIT(meter)=.001;
    double mixed_translation[4] FISICS_DIM(length) FISICS_UNIT(meter)={.001,-.002,.003,.004};
    for(unsigned i=0;i<4;++i)q->translation[i]=zero_length;
    for(unsigned i=0;i<2;++i)q->rotation[i]=zero_rotation;
    if(n<6) {
        if(n%3==2)q->rotation[n/3]=1;
        else q->translation[n<3?n:n-1]=unit_length;
    } else if(n==6) {
        for(unsigned i=0;i<4;++i)q->translation[i]=mixed_translation[i];
        q->rotation[0]=.005;q->rotation[1]=-.006;
    } else if(n==7)q->translation[0]=q->translation[2]=unit_length;
    else if(n==8)q->translation[1]=q->translation[3]=unit_length;
    else if(n==9) {
        double angle FISICS_DIM(scalar)=.001;
        q->rotation[0]=q->rotation[1]=angle;
        q->translation[2]=-basis->sine*s->length*angle;
        q->translation[3]=basis->cosine*s->length*angle;
    } else q->translation[2]=axial_translation;
}

static void reference_global(const FrameLocalSection *section,unsigned mask,
    const FrameLocalBasis *basis,double out[6][6]) {
    double local[6][6];reference_local(section,local);
    if(mask==3){
        for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j)
            if(i%3 || j%3)local[i][j]=0;
    } else if(mask){
        unsigned r=mask==1?2:5;double pivot=local[r][r];
        for(unsigned i=0;i<6;++i)if(i!=r)for(unsigned j=0;j<6;++j)if(j!=r)
            local[i][j]-=local[i][r]*(local[r][j]/pivot);
        for(unsigned i=0;i<6;++i)local[i][r]=local[r][i]=0;
    }
    double c=basis->cosine,s=basis->sine;
    double T[6][6]={{c,s,0,0,0,0},{-s,c,0,0,0,0},{0,0,1,0,0,0},
        {0,0,0,c,s,0},{0,0,0,-s,c,0},{0,0,0,0,0,1}};
    for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j){
        out[i][j]=0;
        for(unsigned a=0;a<6;++a)for(unsigned b=0;b<6;++b)
            out[i][j]+=T[a][i]*local[a][b]*T[b][j];
    }
}
static void poison(FrameLocalStiffness *k){
    double tt FISICS_DIM(force/length)=123456789;
    double coupling FISICS_DIM(force)=123456789;
    double rr FISICS_DIM(force*length)=123456789;
    for(unsigned i=0;i<4;++i){for(unsigned j=0;j<4;++j)k->tt[i][j]=tt;
        for(unsigned j=0;j<2;++j)k->tr[i][j]=coupling;}
    for(unsigned i=0;i<2;++i){for(unsigned j=0;j<4;++j)k->rt[i][j]=coupling;
        for(unsigned j=0;j<2;++j)k->rr[i][j]=rr;}
}
static double observe_load(const FrameLocalLoads *p,unsigned i){
    if(i%3==2)return p->moment[i/3];
    return p->force[i<3?i:i-1];
}
/* Nonsymmetric tags expose permutations hidden by symmetric beam matrices. */
static int dense_permutation(int emit){
    const unsigned t[4]={0,1,3,4},r[2]={2,5};
    double one_tt FISICS_DIM(force/length)=1;
    double one_coupling FISICS_DIM(force)=1;
    double one_rr FISICS_DIM(force*length)=1;
    FrameLocalStiffness tagged,restored;FrameDense6 dense;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){
        double ordinal FISICS_DIM(scalar)=t[i]*6+t[j]+1;tagged.tt[i][j]=one_tt*ordinal;}
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<2;++j){
        double ordinal FISICS_DIM(scalar)=t[i]*6+r[j]+1;tagged.tr[i][j]=one_coupling*ordinal;}
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<4;++j){
        double ordinal FISICS_DIM(scalar)=r[i]*6+t[j]+1;tagged.rt[i][j]=one_coupling*ordinal;}
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j){
        double ordinal FISICS_DIM(scalar)=r[i]*6+r[j]+1;tagged.rr[i][j]=one_rr*ordinal;}
    frame_dense_pack(&tagged,&dense);
    for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j){
        if(dense.value[i][j]!=(double)(i*6+j+1)){
            fprintf(stderr,"dense pack permutation mismatch\n");return 1;}
        if(emit)printf("M 0 %u %u %.17g\n",i,j,dense.value[i][j]);
        dense.value[i][j]=1000+i*6+j+1;
    }
    restored=tagged;frame_dense_restore_assumed_si(&dense,&restored);
    for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j){
        double observed=observe_coefficient(&restored,i,j);
        if(observed!=(double)(1000+i*6+j+1)){
            fprintf(stderr,"dense restore permutation mismatch\n");return 1;}
        if(emit)printf("M 1 %u %u %.17g\n",i,j,observed);
    }
    return 0;
}
int main(int argc,char **argv){
    (void)argv;
    if(dense_permutation(argc>1))return 1;
    const double inputs[3][4]={{12000000,.02,.0004,2},
        {200000000000,.02,.00006666666666666667,2},{70000000000,.003,.0000012,.75}};
    const double orientations[5][2]={{1,0},{0,1},{-1,0},{.6,.8},{.6,-.8}};
    unsigned cases=0;
    for(unsigned s=0;s<3;++s)for(unsigned mask=0;mask<4;++mask)for(unsigned b=0;b<5;++b){
        unsigned id=(s*4+mask)*5+b;
        FrameLocalSection section;
        section.young=inputs[s][0];section.area=inputs[s][1];
        section.inertia=inputs[s][2];section.length=inputs[s][3];
        FrameLocalBasis basis;basis.cosine=orientations[b][0];basis.sine=orientations[b][1];
        FrameLocalStiffness original,condensed,global,alias,restored;
        frame_local_build(&section,&original);poison(&condensed);poison(&global);poison(&restored);
        frame_local_release(&original,mask&1,mask&2,&condensed);
        frame_local_transform(&condensed,&basis,&global);
        alias=original;frame_local_release(&alias,mask&1,mask&2,&alias);
        frame_local_transform(&alias,&basis,&alias);
        FrameDense6 packed,from_reference;
        for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j)packed.value[i][j]=123456789;
        frame_dense_pack(&global,&packed);
        double expected[6][6],local_reference[6][6];
        reference_global(&section,mask,&basis,expected);reference_local(&section,local_reference);
        for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j)from_reference.value[i][j]=expected[i][j];
        /* Restore independently supplied reference values, not our own packed result. */
        frame_dense_restore_assumed_si(&from_reference,&restored);
        if(argc>1)printf("C %u %u %u %u\n",id,s,mask,b);
        for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j){
            double actual[3]={observe_coefficient(&global,i,j),packed.value[i][j],observe_coefficient(&restored,i,j)};
            for(unsigned path=0;path<3;++path){
                if(!close_value(actual[path],expected[i][j],fabs(expected[i][j]))){
                    fprintf(stderr,"matrix mismatch case %u path %u row %u col %u\n",id,path,i,j);return 1;}
                if(argc>1)printf("K %u %u %u %u %.17g\n",id,path,i,j,actual[path]);
            }
            if(!close_value(observe_coefficient(&alias,i,j),expected[i][j],fabs(expected[i][j]))||
               !close_value(observe_coefficient(&original,i,j),local_reference[i][j],fabs(local_reference[i][j])))return 1;
            if((i==2&&(mask&1))||(i==5&&(mask&2)))
                if(!close_value(actual[0],0,fabs(expected[i][j])))return 1;
        }
        for(unsigned n=0;n<11;++n){
            FrameLocalCoordinates q,local_q,alias_q;sample(n,&section,&basis,&q);
            FrameLocalLoads direct,local_p,covariant,restored_p,alias_p;
            frame_local_apply(&global,&q,&direct);
            frame_coordinates_to_local(&q,&basis,&local_q);
            frame_local_apply(&condensed,&local_q,&local_p);
            frame_loads_to_global(&local_p,&basis,&covariant);
            frame_local_apply(&restored,&q,&restored_p);
            alias_q=q;frame_coordinates_to_local(&alias_q,&basis,&alias_q);
            frame_local_apply(&condensed,&alias_q,&alias_p);
            frame_loads_to_global(&alias_p,&basis,&alias_p);
            double raw_q[6]={q.translation[0],q.translation[1],q.rotation[0],q.translation[2],q.translation[3],q.rotation[1]};
            if(argc>1)printf("Q %u %u %.17g %.17g %.17g %.17g %.17g %.17g\n",id,n,raw_q[0],raw_q[1],raw_q[2],raw_q[3],raw_q[4],raw_q[5]);
            for(unsigned i=0;i<6;++i){
                double value=0,bound=0;
                for(unsigned j=0;j<6;++j){double term=expected[i][j]*raw_q[j];value+=term;bound+=fabs(term);}
                double actual[4]={observe_load(&direct,i),observe_load(&covariant,i),observe_load(&restored_p,i),observe_load(&alias_p,i)};
                for(unsigned path=0;path<4;++path){
                    if(!close_value(actual[path],value,bound)){
                        fprintf(stderr,"application mismatch case %u sample %u path %u row %u\n",id,n,path,i);return 1;}
                    if(n>=7&&n<=9&&!close_value(actual[path],0,bound))return 1;
                    if(argc>1)printf("P %u %u %u %u %.17g\n",id,n,path,i,actual[path]);
                }
            }
        }
        ++cases;
    }
    printf("Frame boundary: %u cases, 2160 coefficient positions, 660 applications; PASS\n",cases);
    return 0;
}
