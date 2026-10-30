#include "core_structural.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int main(void) {
    printf("ABI %zu %zu %zu %zu %zu\n",sizeof(CoreStructuralModel),sizeof(CoreStructuralFrameResult),sizeof(CoreStructuralMemberResult),offsetof(CoreStructuralFrameResult,members),offsetof(CoreStructuralMemberResult,end_forces));
    for(unsigned case_id=0;case_id<9;++case_id) {
        double length=1.0+(case_id%3)*1.5;
        double force=case_id<3?1000:case_id<6?0:1000;
        double line=case_id<3?0:250;
        CoreStructuralModel *m=calloc(1,sizeof(*m)); if(!m)return 1;
        core_structural_model_init(m);
        m->node_count=2;m->material_count=1;m->section_count=1;m->member_count=1;m->support_count=1;
        m->nodes[0]=(CoreStructuralNode){"a",0,0};m->nodes[1]=(CoreStructuralNode){"b",length,0};
        m->materials[0]=(CoreStructuralMaterial){"generic",200e9,.25,600,1e7,1e7,1e6,CORE_STRUCTURAL_ISOTROPIC,0,0,0,0,0,0,0,0};
        m->sections[0]=(CoreStructuralSection){"rect",.1,.2};
        m->members[0]=(CoreStructuralMember){"beam","a","b","generic","rect",false,false,0};
        m->supports[0]=(CoreStructuralSupport){"anchor","a",7};m->load_count=2;
        m->loads[0]=(CoreStructuralLoad){"force","b",CORE_STRUCTURAL_NODAL,{0,-force,0,0,0,0},0,false,false,{0}};
        m->loads[1]=(CoreStructuralLoad){"uniform","beam",CORE_STRUCTURAL_UNIFORM_LOCAL,{0,-line,0,0,0,0},0,false,false,{0}};
        CoreStructuralDiagnostic d;CoreStructuralAnalysis *a=NULL;CoreStructuralFrameResult r;
        if(core_structural_analysis_create(m,&a,&d)!=CORE_STRUCTURAL_OK)return 2;
        if(core_structural_analysis_solve_static(a,&d)!=CORE_STRUCTURAL_OK){fprintf(stderr,"solve: %s\n",d.message);return 3;}
        if(core_structural_analysis_read_result(a,&r,&d)!=CORE_STRUCTURAL_OK)return 4;
        printf("C %u %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g",case_id,r.nodes[1].uy_m,r.nodes[1].rz_rad,r.supports[0].fy_n,r.supports[0].mz_nm,r.members[0].max_tension_pa,r.members[0].max_compression_pa,r.members[0].max_shear_pa,r.strain_energy_j,r.external_work_j,r.members[0].max_chord_deflection_m);
        for(unsigned i=0;i<6;++i)printf(" %.17g",r.members[0].end_forces[i]);
        printf("\n");
        double k[36];if(core_structural_frame_stiffness(m,"beam",k,&d)!=CORE_STRUCTURAL_OK)return 5;
        for(unsigned i=0;i<36;++i)printf("K %u %u %.17g\n",case_id,i,k[i]);
        core_structural_analysis_destroy(a);free(m);
    }
    return 0;
}
