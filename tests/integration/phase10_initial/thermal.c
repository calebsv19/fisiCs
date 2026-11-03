/* Test-only accepted-state observer; no solver or reference update here. */
#include "core_thermal_material_rod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"thermal adapter line %d: %s\n",__LINE__,#x);return 2;} } while(0)
int main(int argc,char **argv){
 CHECK(argc==3||argc==5);int kind=atoi(argv[1]),variant=atoi(argv[2]);
 CHECK(kind>=0&&kind<=2&&variant>=0&&variant<(kind==2?8:2));
 double dt=kind==0?.25:kind==1?.1:1./1600;unsigned ticks=kind==0?4:kind==1?100:5120;
 if(argc==5){char *end=NULL;dt=strtod(argv[3],&end);CHECK(end&&!*end&&isfinite(dt));ticks=(unsigned)strtoul(argv[4],&end,10);CHECK(end&&!*end);
  double base=kind==0?.25:kind==1?.1:1./1600;unsigned base_ticks=kind==0?4:kind==1?100:5120;int valid=0;
  for(unsigned j=0;j<4;j++)if(dt==base/(1u<<j)&&ticks==base_ticks*(1u<<j))valid=1;
  if(dt==base&&(ticks==100||ticks==1000||ticks==10000))valid=1;CHECK(valid);
 }
 unsigned pulse0=(unsigned)llround(1.28/dt),pulse1=(unsigned)llround(2.56/dt);
 CoreThermalRod m={0};CoreThermalMaterialRod mm={0};CoreThermalRodState state={0};
 double initial[2]={10,0},amplitudes[]={1e-16,1e-12,1e-100,1e-200,1e-16,1e-16,1e-16,1e-16};
 unsigned count=kind==0?1:2;
 if(kind==0){m.count=1;m.area_m2=2;m.width_m[0]=3;m.capacity_j_per_m3_k[0]=4;m.conductivity_w_per_m_k[0]=5;}
 if(kind==1){m.count=2;m.area_m2=1;initial[0]=variant?333.4:400;initial[1]=variant?333.3:300;for(unsigned i=0;i<2;i++){m.width_m[i]=.5;m.capacity_j_per_m3_k[i]=i?400:200;m.conductivity_w_per_m_k[i]=5;}}
 if(kind==2){mm.count=2;mm.area_m2=.0625;initial[0]=initial[1]=300;for(unsigned i=0;i<2;i++){mm.width_m[i]=.005;mm.material[i]=(CoreThermalMaterial){300,1200,160000,160000,.16,.16};}if(variant==7){mm.material[1].cv_min_j_per_m3_k=mm.material[1].cv_max_j_per_m3_k=320000;mm.material[1].conductivity_min_w_per_m_k=mm.material[1].conductivity_max_w_per_m_k=.32;}}
 CHECK((kind==2?core_thermal_material_rod_init(&mm,initial,&state):core_thermal_rod_init(&m,initial,&state))==CORE_THERMAL_OK);
 if(kind==2&&variant==6)state.energy_j[0]=amplitudes[variant];
 printf("{\"kind\":%d,\"variant\":%d,\"dt\":%.17g,\"ticks\":%u,\"count\":%u,\"initial_e\":[%.17g,%.17g]}\n",kind,variant,dt,ticks,count,state.energy_j[0],state.energy_j[1]);
 for(unsigned n=0;n<ticks;n++){
  double power[2]={0,0};if(kind==0)power[0]=variant?.012:12;
  if(kind==2){double a=amplitudes[variant];power[0]=variant==6?0:a;if(variant==4){power[0]=0;power[1]=a;}if(variant==5)power[0]=n<pulse0?a:n<pulse1?0:-a/4;}
  CoreThermalRodState saved=state;CoreThermalRodStep r;unsigned iterations=0;
  if(kind==2){CoreThermalMaterialRodStep candidate;CHECK(core_thermal_material_rod_step(&mm,&state,power,dt,&candidate)==CORE_THERMAL_OK);r=candidate.result;iterations=candidate.iterations;}else CHECK(core_thermal_rod_step(&m,&state,power,dt,&r)==CORE_THERMAL_OK);
  CHECK(!memcmp(&saved,&state,sizeof state));
  printf("{\"tick\":%u,\"time\":%.17g,\"e\":[%.17g,%.17g],\"t\":[%.17g,%.17g],\"p\":[%.17g,%.17g],\"face\":[%.17g,%.17g,%.17g],\"boundary\":[%.17g,%.17g],\"source\":%.17g,\"sink\":%.17g,\"stored\":%.17g,\"residual\":%.17g,\"iterations\":%u}\n",n+1,(n+1)*dt,r.state.energy_j[0],r.state.energy_j[1],r.temperature_k[0],r.temperature_k[1],power[0],power[1],r.face_transfer_j[0],r.face_transfer_j[1],r.face_transfer_j[2],r.boundary_into_j[0],r.boundary_into_j[1],r.source_j,r.sink_j,r.stored_change_j,r.balance_residual_j,iterations);
  state=r.state;
 }
 return 0;
}
