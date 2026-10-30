#include "core_structural.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"joint energy line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int near(double a,double b){return isfinite(a)&&fabs(a-b)<=1e-8+1e-8*fabs(b);}
int main(void){
 CoreStructuralModel *m=calloc(1,sizeof(*m));CHECK(m);core_structural_model_init(m);
 m->node_count=3;m->material_count=1;m->section_count=1;m->member_count=1;m->support_count=1;m->load_count=1;m->connection_count=1;
 m->nodes[0]=(CoreStructuralNode){"a",0,0};m->nodes[1]=(CoreStructuralNode){"b",0,0};m->nodes[2]=(CoreStructuralNode){"c",2,0};
 m->materials[0]=(CoreStructuralMaterial){"generic",200e9,.25,600,1e7,1e7,1e6,CORE_STRUCTURAL_ISOTROPIC,0,0,0,0,0,0,0,0};
 m->sections[0]=(CoreStructuralSection){"rect",.1,.2};m->members[0]=(CoreStructuralMember){"beam","b","c","generic","rect",false,false,0};
 m->supports[0]=(CoreStructuralSupport){"anchor","a",7};
 m->connections[0]=(CoreStructuralConnection){"spring","a","b",CORE_STRUCTURAL_ELASTIC,7,{1e6,1e6,1e4,0,0,0},{0}};
 m->loads[0]=(CoreStructuralLoad){"force","c",CORE_STRUCTURAL_NODAL,{0,-1000,-200,0,0,0},0,false,false,{0}};
 CoreStructuralDiagnostic d;CoreStructuralAnalysis *a=NULL;CoreStructuralFrameResult r;
 CHECK(core_structural_analysis_create(m,&a,&d)==CORE_STRUCTURAL_OK);
 CoreStructuralStatus status=core_structural_analysis_solve_static(a,&d);
 if(status!=CORE_STRUCTURAL_OK)fprintf(stderr,"solve: %s\n",d.message);
 CHECK(status==CORE_STRUCTURAL_OK);CHECK(core_structural_analysis_read_result(a,&r,&d)==CORE_STRUCTURAL_OK);
 /* Independent integration of (P*(L-x)+M)^2/(2EI), plus two spring stores. */
 double ei=200e9*(.1*.2*.2*.2/12);
 double member=(1000.0*1000*8/3+1000.0*200*4+200.0*200*2)/(2*ei);
 double joint=.5*(1000.0*1000/1e6+2200.0*2200/1e4);
 CHECK(near(r.member_strain_energy_j,member));CHECK(near(r.connection_strain_energy_j,joint));
 CHECK(near(r.strain_energy_j,member+joint));CHECK(near(r.external_work_j,2*(member+joint)));
 CHECK(near(r.algebraic_strain_energy_j,member+joint));CHECK(near(r.algebraic_external_work_j,r.external_work_j));
 CHECK(near(r.connections[0].force_on_a[1],-1000));CHECK(near(r.connections[0].force_on_a[2],-2200));
 CHECK(near(r.connections[0].relative_displacement[1],-.001));CHECK(near(r.connections[0].relative_displacement[2],-.22));
 CHECK(near(r.supports[0].fy_n,1000));CHECK(near(r.supports[0].mz_nm,2200));
 printf("member %.17g joint %.17g total %.17g endpoint %.17g\n",r.member_strain_energy_j,r.connection_strain_energy_j,r.strain_energy_j,r.external_work_j);
 core_structural_analysis_destroy(a);free(m);return 0;
}
