// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include "15__probe_osp3_raw_storage_policy.h"
#ifndef OSP3_STORE_MODE
#define OSP3_STORE_MODE 0
#endif
#ifndef OSP3_STORE_SEED
#define OSP3_STORE_SEED 0x93d765a1U
#endif
#ifndef OSP3_STORE_BUDGET
#define OSP3_STORE_BUDGET 256U
#endif
#define CAP 512U
#define TOTAL 128U
struct Stats{unsigned cases,accept,reject,fail,digest;};
static void w32(unsigned char*p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
static void w64(unsigned char*p,unsigned long v){w32(p,(unsigned)v);w32(p+4,(unsigned)(v>>32));}
static unsigned fnv(const unsigned char*p,unsigned n){unsigned v=0x811c9dc5U,i;for(i=0;i<n;i++)v=(v^p[i])*0x01000193U;return v;}
static void zero(unsigned char*p,unsigned n){unsigned i;for(i=0;i<n;i++)p[i]=0;}
static void seal(unsigned char*p){w32(p+48,fnv(p+64,TOTAL-64));w32(p+52,fnv(p,52));}
static void make(unsigned char*p){unsigned i;zero(p,CAP);w64(p,0x31474d4953575346UL);w32(p+8,1);w32(p+12,64);w32(p+16,TOTAL);w32(p+20,4096);w32(p+24,64);w32(p+28,48);w32(p+32,16);w32(p+36,9);w32(p+40,2);w32(p+64,8);w32(p+68,8);w32(p+72,1);w32(p+80,16);w32(p+84,8);w32(p+88,2);for(i=96;i<TOTAL;i++)p[i]=(unsigned char)(i*17U+3U);seal(p);}
static unsigned reset(const struct Osp3StoreState*s){unsigned i;if(s->accepted||s->sequence||s->extent_count||s->journal_start||s->journal_end||s->data_blocks||s->table_digest)return 0;for(i=0;i<OSP3_STORE_MAX_EXTENTS;i++)if(s->start[i]||s->end[i])return 0;return 1;}
static void mix(struct Stats*s,unsigned v){s->digest=(s->digest^v)*0x01000193U;}
static void rec(struct Stats*s,const unsigned char*p,unsigned long n,unsigned floor,unsigned ea,unsigned er){struct Osp3StoreState st;int a=osp3_raw_storage_admit(p,n,floor,&st);s->cases++;if(a)s->accept++;else s->reject++;if((unsigned)a!=ea||st.reason!=er||(!a&&!reset(&st))||(a&&(!st.extent_count||!st.data_blocks)))s->fail++;mix(s,(unsigned)a|((st.reason<<1)^st.sequence^st.table_digest^st.data_blocks));}
static void obs(struct Stats*s,const unsigned char*p){struct Osp3StoreState st;int a=osp3_raw_storage_admit(p,TOTAL,6,&st);s->cases++;if(a)s->accept++;else s->reject++;if((!a&&!reset(&st))||(a&&!st.extent_count))s->fail++;mix(s,(unsigned)a|((st.reason<<1)^st.sequence^st.table_digest));}
static void run(unsigned mode,unsigned seed,unsigned budget,struct Stats*s){unsigned char p[CAP];unsigned i,v=seed;make(p);
 if(mode==0){for(i=0;i<16;i++){w32(p+36,9+i);seal(p);rec(s,p,TOTAL,6,1,OSP3_STORE_ACCEPT);}}
 else if(mode==1){for(i=0;i<TOTAL;i++)rec(s,p,i,6,0,i<64?OSP3_STORE_SHORT:OSP3_STORE_GEOMETRY);rec(s,p,TOTAL,6,1,OSP3_STORE_ACCEPT);}
 else if(mode==2){p[0]^=1;p[8]=2;p[100]^=1;rec(s,p,TOTAL,99,0,OSP3_STORE_IDENT);make(p);p[8]=2;p[100]^=1;rec(s,p,TOTAL,99,0,OSP3_STORE_VERSION);make(p);w32(p+20,512);p[100]^=1;rec(s,p,TOTAL,99,0,OSP3_STORE_GEOMETRY);make(p);p[51]^=1;p[100]^=1;rec(s,p,TOTAL,99,0,OSP3_STORE_HEADER_CHECKSUM);make(p);p[100]^=1;rec(s,p,TOTAL,99,0,OSP3_STORE_TABLE_CHECKSUM);make(p);rec(s,p,TOTAL,9,0,OSP3_STORE_REPLAY);}
 else if(mode==3){rec(s,p,TOTAL,6,1,OSP3_STORE_ACCEPT);rec(s,p,TOTAL,9,0,OSP3_STORE_REPLAY);w32(p+36,0);seal(p);rec(s,p,TOTAL,0,0,OSP3_STORE_REPLAY);make(p);w32(p+36,0xffffffffU);seal(p);rec(s,p,TOTAL,0xfffffffeU,1,OSP3_STORE_ACCEPT);}
 else if(mode==4){w32(p+64,1);seal(p);rec(s,p,TOTAL,6,0,OSP3_STORE_EXTENT_RANGE);make(p);w32(p+68,0);seal(p);rec(s,p,TOTAL,6,0,OSP3_STORE_EXTENT_RANGE);make(p);w32(p+64,63);seal(p);rec(s,p,TOTAL,6,0,OSP3_STORE_EXTENT_RANGE);make(p);w32(p+64,48);seal(p);rec(s,p,TOTAL,6,0,OSP3_STORE_JOURNAL_OVERLAP);make(p);w32(p+80,12);seal(p);rec(s,p,TOTAL,6,0,OSP3_STORE_EXTENT_OVERLAP);make(p);rec(s,p,TOTAL,6,1,OSP3_STORE_ACCEPT);}
 else if(mode==5){for(i=0;i<64;i++){make(p);if(i&1U){w32(p+80,12);seal(p);rec(s,p,TOTAL,6,0,OSP3_STORE_EXTENT_OVERLAP);}else rec(s,p,TOTAL,6,1,OSP3_STORE_ACCEPT);}}
 else for(i=0;i<budget;i++){unsigned off;make(p);v=v*1664525U+1013904223U;off=v%TOTAL;p[off]^=(unsigned char)(1U<<((v>>24)&7U));if((i&3U)==0)seal(p);obs(s,p);}
}
int main(void){struct Stats s={0,0,0,0,0x811c9dc5U};run(OSP3_STORE_MODE,OSP3_STORE_SEED,OSP3_STORE_BUDGET,&s);printf("OSP3 raw-storage mode=%u seed=%08x budget=%u cases=%u accept=%u reject=%u failures=%u digest=%u\n",(unsigned)OSP3_STORE_MODE,(unsigned)OSP3_STORE_SEED,(unsigned)OSP3_STORE_BUDGET,s.cases,s.accept,s.reject,s.fail,s.digest);return s.fail?1:0;}
