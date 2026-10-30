#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fisics_frontend.h"
typedef struct {const char *name,*old_text,*new_text,*definitions,*body;int code;} Case;
static const Case cases[]={
    {"build_tt_class","k->tt[0][0] = axial;","k->tt[0][0] = rotation_diagonal;","","",4103},
    {"build_tr_class","k->tr[1][0] = coupling;","k->tr[1][0] = bending;","","",4103},
    {"build_rt_class","k->rt[0][1] = coupling;","k->rt[0][1] = rotation_diagonal;","","",4103},
    {"build_rr_class","k->rr[0][0] = rotation_diagonal;","k->rr[0][0] = axial;","","",4103},
    {"axial_missing_division","section->young * section->area / section->length;","section->young * section->area * section->length;","","",4103},
    {"bending_missing_length","(section->length * section->length * section->length);","(section->length * section->length);","","",4103},
    {"apply_tt_rotation","k->tt[i][j] * q->translation[j];","k->tt[i][j] * q->rotation[j%2];","","",4101},
    {"apply_tr_translation","k->tr[i][j] * q->rotation[j];","k->tr[i][j] * q->translation[j];","","",4101},
    {"apply_rt_rotation","k->rt[i][j] * q->translation[j];","k->rt[i][j] * q->rotation[j%2];","","",4101},
    {"apply_rr_translation","k->rr[i][j] * q->rotation[j];","k->rr[i][j] * q->translation[j];","","",4101},
    {"complete_kernel","","","","FrameLocalSection s;FrameLocalStiffness k;FrameLocalCoordinates q;FrameLocalLoads p;frame_local_build(&s,&k);frame_local_apply(&k,&q,&p);",0},
    {"translation_from_rotation","","","","FrameLocalCoordinates q;q.translation[0]=q.rotation[0];",4103},
    {"rotation_from_translation","","","","FrameLocalCoordinates q;q.rotation[0]=q.translation[0];",4103},
    {"force_from_moment","","","","FrameLocalLoads p;p.force[0]=p.moment[0];",4103},
    {"moment_from_force","","","","FrameLocalLoads p;p.moment[0]=p.force[0];",4103},
    {"dynamic_block_swap","","","","FrameLocalStiffness k;unsigned i=1,j=1;k.tt[i][j]=k.rr[i][j];",4103},
    {"dynamic_block_valid","","","","FrameLocalStiffness k;unsigned i=1,j=1;k.tt[i][j]=k.tt[j][i];",0},
    {"section_unit_write","","","","FrameLocalSection s;double ft [[fisics::dim(length)]]  [[fisics::unit(foot)]] =2;s.length=ft;",4107},
    {"section_unit_read","","","","FrameLocalSection s;double ft [[fisics::dim(length)]]  [[fisics::unit(foot)]] =s.length;",4107},
    {"section_unit_convert","","","","FrameLocalSection s;double ft [[fisics::dim(length)]]  [[fisics::unit(foot)]] =2;s.length=fisics_convert_unit(ft,\"meter\");",0},
    {"force_argument","","","void take(double [[fisics::dim(time)]] );","FrameLocalLoads p;take(p.force[0]);",4112},
    {"rotation_argument","","","void take(double [[fisics::dim(length)]] );","FrameLocalCoordinates q;take(q.rotation[0]);",4112},
    {"moment_return","","","double [[fisics::dim(force)]]  get(FrameLocalLoads *p){return p->moment[0];}","",4113},
    {"force_return_valid","","","double [[fisics::dim(force)]]  get(FrameLocalLoads *p){return p->force[0];}","",0},
    {"block_initializer","","","","FrameLocalStiffness k;double x[2] [[fisics::dim(length)]] ={k.tt[0][0],0};",4103},
    {"pointer_carrier","","","","FrameLocalStiffness k;FrameLocalStiffness *p=&k;FrameLocalStiffness *alias=p;alias->tt[0][0]=alias->rr[0][0];",4103},
    {"raw_slot_unchecked","","","","double k[6][6];k[0][0]=k[2][2];",0},
    {"unknown_input_unchecked","","","double external(void);","FrameLocalStiffness k;k.tt[0][0]=external();",0},
    {"same_dimension_coupling_roles","","","","FrameLocalStiffness k;k.tr[1][0]=k.rt[0][1];",0},
};
int main(void) {
    FILE *file=fopen("tests/fixtures/solver_physics/frame_local.h","rb");
    if(!file)return 1;
    if(fseek(file,0,SEEK_END)!=0)return 1;
    long size=ftell(file);if(size<0||size>65536)return 1;
    rewind(file);char *base=calloc((size_t)size+1,1);
    if(!base||fread(base,1,(size_t)size,file)!=(size_t)size)return 1;
    fclose(file);int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[98304];const Case *c=&cases[i];
        const char *at=c->old_text[0]?strstr(base,c->old_text):NULL;
        if(c->old_text[0]&&(!at||strstr(at+strlen(c->old_text),c->old_text)))return 1;
        if(at)snprintf(source,sizeof(source),"%.*s%s%s\n%s\nint main(void){%s return 0;}\n",
            (int)(at-base),base,c->new_text,at+strlen(c->old_text),c->definitions,c->body);
        else snprintf(source,sizeof(source),"%s\n%s\nint main(void){%s return 0;}\n",base,c->definitions,c->body);
        FisicsFrontendOptions options={0};options.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult result={0};
        int ok=fisics_analyze_buffer("frame_local_contract.c",source,strlen(source),&options,&result);
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
    free(base);printf("Frame-local frontend contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
