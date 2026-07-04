typedef struct S { unsigned int shard, lane, checkpoint, watermark, payload; } S;
typedef struct A { unsigned int checkpoint[4], watermark[4], payload[4]; } A;
static unsigned int mix13(unsigned int h,unsigned int v){ h ^= v + 0x9e3779b9u + (h<<6) + (h>>2); return h; }
void seed13(S* out,unsigned int shard,unsigned int lane,unsigned int checkpoint,unsigned int watermark,unsigned int payload){ out->shard=shard%4u; out->lane=lane%4u; out->checkpoint=checkpoint; out->watermark=watermark; out->payload=payload; }
void encode13(const S* s,unsigned int wire[5]){ wire[0]=s->shard^0x15u; wire[1]=s->lane^0x37u; wire[2]=s->checkpoint^0x59u; wire[3]=s->watermark^0x7bu; wire[4]=s->payload^0x9du; }
void decode13(S* s,const unsigned int wire[5]){ s->shard=wire[0]^0x15u; s->lane=wire[1]^0x37u; s->checkpoint=wire[2]^0x59u; s->watermark=wire[3]^0x7bu; s->payload=wire[4]^0x9du; }
void clear13(A* a){ for(int lane=0; lane<4; ++lane){ a->checkpoint[lane]=0u; a->watermark[lane]=0u; a->payload[lane]=0u; } }
void absorb13(A* a,const S* s){ unsigned int lane=s->lane%4u; if(s->checkpoint<a->checkpoint[lane]) return; if(s->checkpoint>a->checkpoint[lane]){ a->checkpoint[lane]=s->checkpoint; a->watermark[lane]=s->watermark; a->payload[lane]=s->payload; return; } if(s->watermark>a->watermark[lane]){ a->watermark[lane]=s->watermark; a->payload[lane]=s->payload; } else if(s->watermark==a->watermark[lane]) a->payload[lane]+=s->payload; }
unsigned int sig13(const A* a){ unsigned int h=2166136261u; for(int lane=0; lane<4; ++lane){ h=mix13(h,a->checkpoint[lane]); h=mix13(h,a->watermark[lane]); h=mix13(h,a->payload[lane]); } return h; }
