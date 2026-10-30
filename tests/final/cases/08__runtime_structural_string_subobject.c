/* String subobjects must consume one initializer and zero-fill their full extent. */
#include <string.h>
typedef struct Record { char id[48]; double x, y; } Record;
typedef struct Pair { Record records[2]; int sentinel; } Pair;
static int check(const Record *r,const char *id,double x,double y) {
    if(strcmp(r->id,id)||r->x!=x||r->y!=y)return 1;
    for(unsigned i=(unsigned)strlen(id)+1;i<sizeof(r->id);++i)if(r->id[i])return 2;
    return 0;
}
int main(void) {
    Record direct={"first",2,3};
    Record compound;memset(&compound,0x5a,sizeof(compound));
    compound=(Record){"second",4,5};
    Record designated={.y=9,.id="name",.x=8};
    Pair nested={{{"left",10,11},{"right",12,13}},14};
    Pair flat={"flat_left",20,21,"flat_right",22,23,24};
    char exact[4]="abc";unsigned char small[8]="ok";
    if(check(&direct,"first",2,3)||check(&compound,"second",4,5)||check(&designated,"name",8,9))return 1;
    if(check(&nested.records[0],"left",10,11)||check(&nested.records[1],"right",12,13)||nested.sentinel!=14)return 2;
    if(check(&flat.records[0],"flat_left",20,21)||check(&flat.records[1],"flat_right",22,23)||flat.sentinel!=24)return 3;
    if(exact[0]!='a'||exact[1]!='b'||exact[2]!='c'||small[0]!='o'||small[1]!='k')return 4;
    for(unsigned i=2;i<sizeof(small);++i)if(small[i])return 5;
    return 0;
}
