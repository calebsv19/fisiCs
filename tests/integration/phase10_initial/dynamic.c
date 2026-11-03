/* Only observes the existing admitted elastic axial owner. */
#include "core_structural_frame_motion.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static CoreStructuralDiagnostic diagnostic;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"dynamic adapter line %d: %s [%s]\n",__LINE__,#x,diagnostic.message);exit(2);}}while(0)
static void vector(const double *a){printf("[");for(unsigned i=0;i<6;i++)printf("%s%.17g",i?",":"",a[i]);printf("]");}
static void observe(const CoreStructuralFrameMotionState *r){
 printf("{\"tick\":%llu,\"time\":%.17g",(unsigned long long)r->tick,r->time_s);
 const double *v[]={r->frame.state.displacement,r->velocity,r->acceleration,r->frame.state.applied,r->inertial,r->damping,r->frame.state.reaction,r->frame.state.internal};const char *names[]={"u","v","a","force","inertial","damping","reaction","internal"};
 for(unsigned j=0;j<8;j++){printf(",\"%s\":",names[j]);vector(v[j]);}
 double plastic=0;for(unsigned p=0;p<4;p++)plastic=fmax(plastic,r->frame.state.material[p].accumulated_plastic_strain);
 printf(",\"I\":%.17g,\"I0\":%.17g,\"K\":%.17g,\"D\":%.17g,\"W\":%.17g,\"R\":%.17g,\"residual_n\":%.17g,\"residual_nm\":%.17g,\"plastic\":%.17g}\n",r->frame.state.internal_work_j,r->initial_internal_work_j,r->kinetic_j,r->damping_work_j,r->external_work_j,r->energy_balance_j,r->frame.state.residual_n,r->frame.state.residual_nm,plastic);
}
int main(int argc,char **argv){
 CHECK(argc==3||argc==5);int kind=atoi(argv[1]),variant=atoi(argv[2]);CHECK(kind>=0&&kind<3&&variant>=0&&variant<2);
 CoreStructuralPlasticFrameModel m={0};m.nodes=2;m.members=1;m.fibers=2;m.corotational=true;m.maximum_chord_rotation_rad=.75;m.x_m[1]=2;for(unsigned i=0;i<6;i++)m.fixed[i]=i!=3;m.member[0].node_b=1;
 for(unsigned f=0;f<2;f++){m.member[0].fiber_y_m[f]=f?.005:-.005;m.member[0].fiber_area_m2[f]=.0005;m.member[0].material[f]=(CoreStructuralUniaxialMaterial){2e11,1e9,5e9};}
 CoreStructuralFrameMotionSettings s={.dt_s=.0008,.maximum_ticks=200,.alpha_m_per_s=kind?8:0};s.density_kg_m3[0]=7850;s.added_mass_kg[1]=1000;
 if(argc==5){char *end=NULL;s.dt_s=strtod(argv[3],&end);CHECK(end&&!*end&&isfinite(s.dt_s));s.maximum_ticks=strtoull(argv[4],&end,10);CHECK(end&&!*end);int valid=0;
  for(unsigned j=0;j<4;j++)if(s.dt_s==.0008/(1u<<j)&&s.maximum_ticks==200u*(1u<<j))valid=1;
  if(s.dt_s==.0001&&(s.maximum_ticks==100||s.maximum_ticks==1000||s.maximum_ticks==8000))valid=1;CHECK(valid);
 }
 double held[27]={0},released[27]={0},force[27]={0};held[3]=kind==2?-100:0;released[3]=kind==2?0:variant?.1:100;double amplitude=kind==2?(variant?.01:10):0,omega=.8*sqrt(1e8/1007.85);
 CoreStructuralFrameVibration *o=NULL;CHECK(core_structural_frame_vibration_create_preloaded(&m,&s,held,released,&o,&diagnostic)==CORE_STRUCTURAL_OK);CoreStructuralFrameMotionState r;
 CHECK(core_structural_frame_vibration_read(o,&r,&diagnostic)==CORE_STRUCTURAL_OK);
 printf("{\"kind\":%d,\"variant\":%d,\"dt\":%.17g,\"ticks\":%u,\"held\":%.17g,\"released\":%.17g,\"amplitude\":%.17g,\"omega\":%.17g,\"alpha\":%.17g}\n",kind,variant,s.dt_s,(unsigned)s.maximum_ticks,held[3],released[3],amplitude,omega,s.alpha_m_per_s);observe(&r);
 for(unsigned n=0;n<s.maximum_ticks;n++){force[3]=amplitude*sin(omega*((n+1)*s.dt_s));CHECK((kind==2?core_structural_frame_vibration_step_forced(o,n,force,NULL,NULL,&r,&diagnostic):core_structural_frame_vibration_step(o,n,NULL,NULL,&r,&diagnostic))==CORE_STRUCTURAL_OK);observe(&r);}
 core_structural_frame_vibration_destroy(o);return 0;
}
