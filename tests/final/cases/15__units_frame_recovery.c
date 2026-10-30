#include "../../fixtures/solver_physics/frame_recovery.h"
#include "../../fixtures/solver_physics/frame_joint.h"
#include <stdio.h>
/* Numerical observations below deliberately use unannotated doubles. They are
 * test instrumentation, not a production adapter or a physical checksum. */
static double observe_coefficient(const FrameLocalStiffness *k,unsigned i,unsigned j){
    unsigned ti=i<3?i:i-1,tj=j<3?j:j-1;
    if(i%3==2){if(j%3==2)return k->rr[i/3][j/3];return k->rt[i/3][tj];}
    if(j%3==2)return k->tr[ti][j/3];return k->tt[ti][tj];
}
static int close_value(double a,double b,double scale){
    return isfinite(a)&&isfinite(b)&&isfinite(scale)&&fabs(a-b)<=1e-8+1e-9*scale;
}
static int values(const char *kind,unsigned id,const double *v,unsigned count,int emit){
    for(unsigned i=0;i<count;++i){if(!isfinite(v[i]))return 1;
        if(emit)printf("%s %u %u %.17g\n",kind,id,i,v[i]);}
    return 0;
}
static void retained(unsigned pattern,FrameLocalCoordinates *q){
    const double data[2][6]={{.001,-.002,.003,.004,.005,-.006},{-.002,.003,-.001,.002,-.004,.007}};
    for(unsigned i=0;i<4;++i)q->translation[i]=data[pattern][i];
    for(unsigned i=0;i<2;++i)q->rotation[i]=data[pattern][4+i];
}
int main(int argc,char **argv){
    (void)argv;int emit=argc>1;
    const double sections[3][5]={{12000000,.02,.0004,2,.002},
        {200000000000,.02,.00006666666666666667,2,.0006666666666666667},
        {70000000000,.003,.0000012,.75,.00003}};
    const double orientations[3][2]={{1,0},{.6,.8},{.6,-.8}};
    unsigned bases=0,members=0,joints=0;
    for(unsigned s=0;s<3;++s)for(unsigned mask=0;mask<4;++mask)for(unsigned b=0;b<3;++b){
        unsigned id=(s*4+mask)*3+b;
        FrameLocalSection section;section.young=sections[s][0];section.area=sections[s][1];
        section.inertia=sections[s][2];section.length=sections[s][3];
        double modulus FISICS_DIM(length*length*length)=sections[s][4];
        FrameLocalBasis basis;basis.cosine=orientations[b][0];basis.sine=orientations[b][1];
        FrameLoadInput input;
        double density FISICS_DIM(mass/length/length/length)=600;
        double gravity[2] FISICS_DIM(acceleration)={.5,-9.81};input.density=density;
        for(unsigned i=0;i<2;++i)input.gravity[i]=gravity[i];
        double global_line[2] FISICS_DIM(force/length)={12,-34};
        double local_line[2] FISICS_DIM(force/length)={-7,11};
        for(unsigned i=0;i<2;++i){input.global.component[i]=global_line[i];input.local.component[i]=local_line[i];}
        FrameLineLoad line;frame_uniform_local(&section,&basis,&input,&line);
        FrameLocalStiffness k;frame_local_build(&section,&k);
        FrameLocalLoads equivalent,condensed,world,nodal,combined,alias;
        frame_equivalent_load(&section,&line,&equivalent);
        frame_condensed_load(&k,&equivalent,mask&1,mask&2,&condensed);
        alias=equivalent;frame_condensed_load(&k,&alias,mask&1,mask&2,&alias);
        frame_loads_to_global(&condensed,&basis,&world);
        double nodal_f[4] FISICS_DIM(force)={1,-2,4,5};double nodal_m[2] FISICS_DIM(force*length)={3,-6};
        for(unsigned i=0;i<4;++i)nodal.force[i]=nodal_f[i];
        for(unsigned i=0;i<2;++i)nodal.moment[i]=nodal_m[i];
        frame_add_loads(&world,&nodal,&combined);
        FrameLocalLoads sum_alias=world;frame_add_loads(&sum_alias,&nodal,&sum_alias);
        double base_values[26]={line.component[0],line.component[1],
            equivalent.force[0],equivalent.force[1],equivalent.moment[0],equivalent.force[2],equivalent.force[3],equivalent.moment[1],
            condensed.force[0],condensed.force[1],condensed.moment[0],condensed.force[2],condensed.force[3],condensed.moment[1],
            world.force[0],world.force[1],world.moment[0],world.force[2],world.force[3],world.moment[1],
            combined.force[0],combined.force[1],combined.moment[0],combined.force[2],combined.force[3],combined.moment[1]};
        if(values("B",id,base_values,26,emit))return 1;
        for(unsigned i=0;i<4;++i)if(alias.force[i]!=condensed.force[i]||sum_alias.force[i]!=combined.force[i])return 1;
        for(unsigned i=0;i<2;++i)if(alias.moment[i]!=condensed.moment[i]||sum_alias.moment[i]!=combined.moment[i])return 1;
        ++bases;
        for(unsigned n=0;n<2;++n){
            FrameLocalCoordinates q,actual,alias_q;retained(n,&q);
            frame_recover_coordinates(&k,&equivalent,&q,mask&1,mask&2,&actual);
            alias_q=q;frame_recover_coordinates(&k,&equivalent,&alias_q,mask&1,mask&2,&alias_q);
            FrameLocalLoads end;frame_recover_end(&k,&actual,&equivalent,&end);
            FrameMemberFields f;frame_recover_fields(&section,modulus,&line,&actual,&f);
            double end_work FISICS_DIM(energy)=frame_nodal_work(&actual,&end);
            FrameLocalBasis inverse=basis;inverse.sine=-basis.sine;
            FrameLocalCoordinates global_actual;frame_coordinates_to_local(&actual,&inverse,&global_actual);
            double supplied_work FISICS_DIM(energy)=frame_nodal_work(&global_actual,&nodal);
            double equilibrium_work FISICS_DIM(energy)=end_work+f.distributed_work;
            if(!close_value(2*f.energy,equilibrium_work,fabs(end_work)+fabs(f.distributed_work)+2*f.energy)){
                fprintf(stderr,"member energy mismatch %u %u\n",id,n);return 1;}
            for(unsigned i=0;i<4;++i)if(alias_q.translation[i]!=actual.translation[i])return 1;
            for(unsigned i=0;i<2;++i)if(alias_q.rotation[i]!=actual.rotation[i])return 1;
            double end_scale=fabs(end.moment[0])+fabs(end.moment[1])+fabs(equivalent.moment[0])+fabs(equivalent.moment[1]);
            if((mask&1)&&!close_value(end.moment[0],0,end_scale))return 1;
            if((mask&2)&&!close_value(end.moment[1],0,end_scale))return 1;
            double v[34]={actual.translation[0],actual.translation[1],actual.rotation[0],actual.translation[2],actual.translation[3],actual.rotation[1],
                end.force[0],end.force[1],end.moment[0],end.force[2],end.force[3],end.moment[1],
                f.axial_displacement[0],f.axial_displacement[1],f.axial_displacement[2],
                f.transverse_displacement[0],f.transverse_displacement[1],f.transverse_displacement[2],f.transverse_displacement[3],f.transverse_displacement[4],
                f.axial[0],f.axial[1],f.moment[0],f.moment[1],f.moment[2],f.shear[0],f.shear[1],
                f.tension,f.compression,f.max_shear,f.energy,f.distributed_work,end_work,supplied_work};
            if(values("R",id*2+n,v,34,emit))return 1;
            ++members;
        }
    }
    for(unsigned mask=0;mask<8;++mask)for(unsigned n=0;n<2;++n){
        unsigned id=mask*2+n;FrameLocalCoordinates q;retained(n,&q);
        FrameJointStiffness spring;
        double kt[2] FISICS_DIM(force/length)={120000,30000};
        double kr FISICS_DIM(force*length)=800;
        for(unsigned i=0;i<2;++i)spring.translation[i]=kt[i];spring.rotation=kr;
        FrameLocalStiffness k;frame_joint_build(&spring,mask,&k);
        FrameJointResponse response;frame_joint_response(&spring,mask,&q,&response);
        FrameLocalLoads end;frame_local_apply(&k,&q,&end);
        double work FISICS_DIM(energy)=frame_nodal_work(&q,&end);
        if(!close_value(2*response.energy,work,2*response.energy))return 1;
        if(!close_value(end.force[0],-response.force_on_a[0],fabs(end.force[0]))||
           !close_value(end.force[1],-response.force_on_a[1],fabs(end.force[1]))||
           !close_value(end.moment[0],-response.moment_on_a,fabs(end.moment[0])))return 1;
        double v[14]={response.relative_translation[0],response.relative_translation[1],response.relative_rotation,
            response.force_on_a[0],response.force_on_a[1],response.moment_on_a,response.energy,work,
            end.force[0],end.force[1],end.moment[0],end.force[2],end.force[3],end.moment[1]};
        if(values("J",id,v,14,emit))return 1;
        for(unsigned i=0;i<6;++i)for(unsigned j=0;j<6;++j){double value=observe_coefficient(&k,i,j);
            if(!isfinite(value))return 1;if(emit)printf("K %u %u %u %.17g\n",id,i,j,value);}
        ++joints;
    }
    /* Original stress oracle plus a physically equilibrated cantilever control. */
    FrameMemberFields point={0};double force FISICS_DIM(force)=120;double moment FISICS_DIM(force*length)=200;
    point.axial[0]=force;point.moment[0]=moment;
    double area FISICS_DIM(area)=.02;double modulus FISICS_DIM(length*length*length)=.002;
    frame_stress_envelope(&point,area,modulus);
    if(!close_value(point.tension,106000,106000)||!close_value(point.compression,94000,94000))return 1;
    FrameLocalSection beam;beam.young=sections[1][0];beam.area=sections[1][1];beam.inertia=sections[1][2];beam.length=sections[1][3];
    FrameLocalStiffness k;frame_local_build(&beam,&k);
    FrameLocalCoordinates q={0};double deflection FISICS_DIM(length) FISICS_UNIT(meter)=-.0002;
    q.translation[3]=deflection;q.rotation[1]=-.00015;
    FrameLineLoad zero_line={0};FrameLocalLoads eq,end;frame_equivalent_load(&beam,&zero_line,&eq);
    frame_recover_end(&k,&q,&eq,&end);FrameMemberFields f;
    double beam_modulus FISICS_DIM(length*length*length)=sections[1][4];frame_recover_fields(&beam,beam_modulus,&zero_line,&q,&f);
    double tip_force FISICS_DIM(force)=-1000;
    double tip_work FISICS_DIM(energy)=tip_force*q.translation[3];
    if(!close_value(end.force[1],1000,1000)||!close_value(end.force[3],-1000,1000)||
       !close_value(end.moment[0],2000,2000)||!close_value(end.moment[1],0,2000)||
       !close_value(f.energy,.1,.1)||!close_value(2*f.energy,tip_work,.2)||
       !close_value(f.tension,3000000,3000000)||!close_value(f.compression,3000000,3000000)){
        fprintf(stderr,"cantilever mismatch\n");return 1;}
    double controls[8]={point.tension,point.compression,end.force[1],end.force[3],end.moment[0],f.energy,tip_work,f.tension};
    if(values("A",0,controls,8,emit))return 1;
    printf("Frame recovery: %u load cases, %u members, %u joints; PASS\n",bases,members,joints);
    return 0;
}
