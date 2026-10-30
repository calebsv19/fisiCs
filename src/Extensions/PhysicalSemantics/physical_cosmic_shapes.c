// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include <string.h>
static bool kind(const FisicsPhysicalPolicy *p,uint32_t k,const char *name) {
 static const struct {const char *name;int8_t dim[8];bool axial;} rows[]={
 {"displacement",{1},false},{"geometric_length",{1},false},
 {"velocity",{1,0,-1},false},{"acceleration",{1,0,-2},false},
 {"angular_rate",{0,0,-1},true},{"specific_angular_momentum",{2,0,-1},true},
 {"orbital_angular_momentum",{2,1,-1},true},{"momentum",{1,1,-1},false},
 {"ratio",{0},false},{"speed_squared",{2,0,-2},false},
 {"eccentricity_numerator",{3,0,-2},false}};
 if(!p || p->schema_version!=7 || !k || k>p->kind_count) return false;
 for(size_t i=0;i<sizeof(rows)/sizeof(rows[0]);i++) if(!strcmp(name,rows[i].name)) {
  const char *id=p->kinds[k-1].id;
  return !strncmp(id,"quantity.",9) && !strcmp(id+9,name) &&
    !memcmp(p->kinds[k-1].dimension,rows[i].dim,8) &&
    p->models[k-1].role==(!strcmp(name,"displacement")?FISICS_PHYSICAL_DIFFERENCE:FISICS_PHYSICAL_LINEAR);
 }
 return false;
}
bool physical_cosmic_vector_kind(const FisicsPhysicalPolicy *p,uint32_t k,bool axial) {
 const char *polar[]={"displacement","velocity","acceleration","momentum","ratio","eccentricity_numerator"};
 const char *axes[]={"angular_rate","specific_angular_momentum","orbital_angular_momentum"};
 const char **rows=axial?axes:polar;size_t n=axial?sizeof(axes)/sizeof(*axes):sizeof(polar)/sizeof(*polar);
 for(size_t i=0;i<n;i++) if(kind(p,k,rows[i]))return true;return false;
}
static bool rep(const FisicsPhysicalPolicy *p,uint32_t r,bool axial) {
 if(!r || r>p->representation_count)return false;
 const FisicsPhysicalRepresentation *v=&p->representations[r-1];
 return v->record && !v->single_precision && v->rank==1 && v->extent==3 && v->axial==axial &&
 v->field_count==3 && !strcmp(v->fields[0],"x") && !strcmp(v->fields[1],"y") && !strcmp(v->fields[2],"z");
}
bool physical_cosmic_shape_contract(const FisicsPhysicalPolicy *p,const FisicsPhysicalShapeHelper *h) {
 if(!p || p->schema_version!=7)return false;
 uint32_t a=h->kinds[0],b=h->kinds[1],k=h->kinds[2];
 bool aa=physical_cosmic_vector_kind(p,a,true),ba=physical_cosmic_vector_kind(p,b,true);
 if(!physical_cosmic_vector_kind(p,a,aa) || !rep(p,h->representations[0],aa))return false;
 if(h->operation==PHYSICAL_SHAPE_NORM)
  return !kind(p,a,"momentum") && !kind(p,a,"eccentricity_numerator") && !b && !h->representations[1] && !h->representations[2] &&
   (kind(p,a,"displacement")?kind(p,k,"geometric_length"):a==k);
 if(!physical_cosmic_vector_kind(p,b,ba) || !rep(p,h->representations[1],ba))return false;
 if(h->operation==PHYSICAL_SHAPE_DOT)
  return !h->representations[2] && ((kind(p,a,"velocity") && a==b && kind(p,k,"speed_squared")) ||
   (kind(p,a,"ratio") && a==b && a==k));
 if(h->operation!=PHYSICAL_SHAPE_CROSS)return false;
 bool valid=(kind(p,a,"displacement") && kind(p,b,"velocity") && kind(p,k,"specific_angular_momentum")) ||
  (kind(p,a,"displacement") && kind(p,b,"momentum") && kind(p,k,"orbital_angular_momentum")) ||
  (kind(p,a,"velocity") && kind(p,b,"specific_angular_momentum") && kind(p,k,"eccentricity_numerator"));

 return valid && rep(p,h->representations[2],aa==ba);
}
