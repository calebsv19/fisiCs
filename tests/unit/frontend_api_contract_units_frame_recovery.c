#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fisics_frontend.h"
typedef struct {const char *name,*old_text,*new_text,*definitions,*body;int code;} Case;
static const Case cases[]={
    {"density_area","input->density*s->area;","input->density;","","",4103},
    {"load_moment_length","q->component[1]*s->length*s->length/12;","q->component[1]*s->length/12;","","",4103},
    {"release_rotation_class","rhs->moment[0]/k->rr[0][0];","rhs->moment[0]/k->tt[0][0];","","",4103},
    {"condensed_load_class","original.force[i]-k->tr[i][0]*rotation.value[0]","original.force[i]-k->rr[i%2][0]*rotation.value[0]","","",4102},
    {"nodal_work_class","q->rotation[i]*p->moment[i];","q->rotation[i]*p->force[i];","","",4101},
    {"joint_tt_class","k->tt[a][a]=s->translation[a];","k->tt[a][a]=s->rotation;","","",4103},
    {"joint_rr_class","k->rr[0][0]=s->rotation;","k->rr[0][0]=s->translation[0];","","",4103},
    {"joint_force_class","s->translation[a]*out->relative_translation[a];","s->rotation*out->relative_translation[a];","","",4103},
    {"joint_energy_length",".5*out->force_on_a[a]*out->relative_translation[a];",".5*out->force_on_a[a];","","",4101},
    {"joint_moment_class","s->rotation*out->relative_rotation;","s->translation[0]*out->relative_rotation;","","",4103},
    {"stress_modulus","sign*f->moment[0]/modulus;","sign*f->moment[0]/area;","","",4101},
    {"shear_length","ei/s->length/s->length/s->length*6*f->transverse_displacement[3];","ei/s->length/s->length*6*f->transverse_displacement[3];","","",4103},
    {"moment_length","ei/s->length/s->length*2*f->transverse_displacement[2];","ei/s->length/s->length/s->length*2*f->transverse_displacement[2];","","",4103},
    {"bubble_length","line->component[1]*s->length*s->length*s->length*s->length/(24*ei);","line->component[1]*s->length*s->length*s->length/(24*ei);","","",4103},
    {"axial_bubble_class","line->component[0]*s->length*s->length/(2*ea);","line->component[0]*s->length*s->length/(2*ei);","","",4103},
    {"member_energy_length","f->energy=s->length*(axial_integral+moment_integral)/2;","f->energy=(axial_integral+moment_integral)/2;","","",4103},
    {"distributed_work_length","f->distributed_work=s->length*work_integral;","f->distributed_work=work_integral;","","",4103},
    {"end_force_class","elastic.force[i]-load.force[i];","elastic.force[i]-load.moment[i%2];","","",4102},
    {"work_return","return work;","return p->force[0];","","",4113},
    {"complete_surface","","","","FrameLoadInput input;FrameLocalSection s;FrameLocalBasis b;FrameLineLoad q;frame_uniform_local(&s,&b,&input,&q);",0},
    {"line_from_moment","","","","FrameLineLoad q;FrameLocalLoads p;q.component[0]=p.moment[0];",4103},
    {"spring_mixed_stiffness","","","","FrameJointStiffness s;s.translation[0]=s.rotation;",4103},
    {"stress_argument","","","","FrameMemberFields f;double area [[fisics::dim(area)]]=1;frame_stress_envelope(&f,area,area);",4112},
    {"energy_argument","","","void take(double x [[fisics::dim(force)]]);","FrameMemberFields f;take(f.energy);",4112},
    {"energy_return","","","double [[fisics::dim(force)]] wrong(FrameJointResponse *p){return p->energy;}","",4113},
    {"translation_unit","","","","FrameLocalCoordinates q;double foot [[fisics::dim(length)]] [[fisics::unit(foot)]]=1;q.translation[0]=foot;",4107},
    {"same_dimension_energy_moment","","","","FrameLocalLoads p;double e [[fisics::dim(energy)]]=p.moment[0];",0},
    {"unknown_load_input","","","double external(void);","FrameLineLoad q;q.component[0]=external();",0},
};
int main(void) {
    const char *paths[]={"tests/fixtures/solver_physics/frame_loading.h","tests/fixtures/solver_physics/frame_joint.h","tests/fixtures/solver_physics/frame_recovery.h"};
    char *base=calloc(65536,1);if(!base)return 1;size_t used=0;
    for(unsigned i=0;i<3;++i){FILE *file=fopen(paths[i],"rb");if(!file)return 1;
        size_t got=fread(base+used,1,65534-used,file);if(ferror(file)||!feof(file))return 1;
        fclose(file);used+=got;base[used++]='\n';base[used]=0;}
    int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[98304];const Case *c=&cases[i];
        const char *at=c->old_text[0]?strstr(base,c->old_text):NULL;
        if(c->old_text[0]&&(!at||strstr(at+strlen(c->old_text),c->old_text)))return 1;
        if(at)snprintf(source,sizeof(source),"%.*s%s%s\n%s\nint main(void){%s return 0;}\n",
            (int)(at-base),base,c->new_text,at+strlen(c->old_text),c->definitions,c->body);
        else snprintf(source,sizeof(source),"%s\n%s\nint main(void){%s return 0;}\n",base,c->definitions,c->body);
        FisicsFrontendOptions options={0};options.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult result={0};
        int ok=fisics_analyze_buffer("tests/fixtures/solver_physics/frame_recovery_contract.c",source,strlen(source),&options,&result);
        size_t errors=0,matching=0;
        for(size_t j=0;j<result.diag_count;++j){
            const FisicsDiagnostic *d=&result.diagnostics[j];
            if(d->severity_id!=FISICS_DIAG_SEVERITY_ERROR)continue;
            ++errors;
            if(d->code_id==c->code&&d->category_id==FISICS_DIAG_CATEGORY_EXTENSION&&d->line>0&&d->column>0)++matching;
            else fprintf(stderr,"%s unexpected %d: %s\n",c->name,d->code_id,d->message);
        }
        size_t expected=c->code?1:0;
        if(!ok||errors!=expected||matching!=errors){fprintf(stderr,"FAIL %s: analyzed=%d errors=%zu expected=%zu\n",c->name,ok,errors,expected);++failures;}
        else printf("PASS %s\n",c->name);
        fisics_free_analysis_result(&result);
    }
    free(base);printf("Frame-recovery frontend contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
