/* Full local matrix parity fixture. Raw reads below are test observations only,
 * not a production dense-storage adapter or a claim of checked raw algebra. */
#include "../../fixtures/solver_physics/frame_local.h"
#include <math.h>
#include <stdio.h>

/* Unannotated numerical reference preserves the original solver's row order
 * and unreleased matrix formula independently of the typed block assignments. */
static void reference(const FrameLocalSection *s, double out[6][6]) {
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
static void sample(unsigned n,const FrameLocalSection *s,FrameLocalCoordinates *q) {
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
        q->translation[3]=s->length*angle;
    } else q->translation[2]=axial_translation;
}
int main(int argc,char **argv) {
    (void)argv;
    const double inputs[3][4]={{12000000,.02,.0004,2},
        {200000000000,.02,.00006666666666666667,2},{70000000000,.003,.0000012,.75}};
    unsigned coefficient_checks=0,application_checks=0;
    for(unsigned c=0;c<3;++c) {
        FrameLocalSection section;
        /* The input table is an explicit raw SI bootstrap for the fixture. */
        section.young=inputs[c][0];section.area=inputs[c][1];
        section.inertia=inputs[c][2];section.length=inputs[c][3];
        FrameLocalStiffness k;
        /* Poison every slot first: construction must overwrite all coefficients,
         * including zero entries, without relying on stack contents. */
        double poison_tt FISICS_DIM(force/length)=123456789;
        double poison_coupling FISICS_DIM(force)=123456789;
        double poison_rr FISICS_DIM(force*length)=123456789;
        for(unsigned i=0;i<4;++i){
            for(unsigned j=0;j<4;++j)k.tt[i][j]=poison_tt;
            for(unsigned j=0;j<2;++j)k.tr[i][j]=poison_coupling;
        }
        for(unsigned i=0;i<2;++i){
            for(unsigned j=0;j<4;++j)k.rt[i][j]=poison_coupling;
            for(unsigned j=0;j<2;++j)k.rr[i][j]=poison_rr;
        }
        frame_local_build(&section,&k);
        double expected[6][6];reference(&section,expected);
        if(argc>1)printf("S %u %.17g %.17g %.17g %.17g\n",c,section.young,section.area,section.inertia,section.length);
        for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j) {
            double actual=observe_coefficient(&k,i,j);
            if(!close_value(actual,expected[i][j],fabs(expected[i][j]))) {
                fprintf(stderr,"coefficient mismatch: section %u row %u col %u\n",c,i,j);return 1;
            }
            ++coefficient_checks;
            if(argc>1)printf("K %u %u %u %.17g\n",c,i,j,actual);
        }
        for(unsigned n=0;n<11;++n) {
            FrameLocalCoordinates q;FrameLocalLoads load;sample(n,&section,&q);
            frame_local_apply(&k,&q,&load);
            double raw_q[6]={q.translation[0],q.translation[1],q.rotation[0],q.translation[2],q.translation[3],q.rotation[1]};
            double raw_p[6]={load.force[0],load.force[1],load.moment[0],load.force[2],load.force[3],load.moment[1]};
            if(argc>1)printf("Q %u %u %.17g %.17g %.17g %.17g %.17g %.17g\n",c,n,raw_q[0],raw_q[1],raw_q[2],raw_q[3],raw_q[4],raw_q[5]);
            for(unsigned i=0;i<6;++i) {
                double value=0,bound=0;
                for(unsigned j=0;j<6;++j){double term=expected[i][j]*raw_q[j];value+=term;bound+=fabs(term);}
                if(!close_value(raw_p[i],value,bound)) {
                    fprintf(stderr,"application mismatch: section %u sample %u row %u\n",c,n,i);return 1;
                }
                if(n>=7&&n<=9&&!close_value(raw_p[i],0,bound)) {
                    fprintf(stderr,"rigid-mode mismatch: section %u sample %u row %u\n",c,n,i);return 1;
                }
                ++application_checks;
                if(argc>1)printf("P %u %u %u %.17g\n",c,n,i,raw_p[i]);
            }
            if(c==0&&n==10&&(!close_value(load.force[2],120,120)||!close_value(load.force[0],-120,120)))return 1;
        }
    }
    printf("Frame local: 3 sections, %u coefficients, 33 applications, %u result components; PASS\n",coefficient_checks,application_checks);
    return 0;
}
