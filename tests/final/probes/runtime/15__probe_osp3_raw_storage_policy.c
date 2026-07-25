// SPDX-License-Identifier: Apache-2.0
#include "15__probe_osp3_raw_storage_policy.h"
#define STORE_MAGIC 0x31474d4953575346UL
#define STORE_HEADER 64U
#define STORE_EXTENT 16U
static osp3_store_u32 r32(const osp3_store_u8*p){return (osp3_store_u32)p[0]|((osp3_store_u32)p[1]<<8)|((osp3_store_u32)p[2]<<16)|((osp3_store_u32)p[3]<<24);}
static osp3_store_u64 r64(const osp3_store_u8*p){return (osp3_store_u64)r32(p)|((osp3_store_u64)r32(p+4)<<32);}
static osp3_store_u32 fnv(const osp3_store_u8*p,osp3_store_u64 n){osp3_store_u32 v=0x811c9dc5U;osp3_store_u64 i;for(i=0;i<n;i++)v=(v^p[i])*0x01000193U;return v;}
static void clear(struct Osp3StoreState*s){osp3_store_u32 i;s->accepted=s->reason=s->sequence=s->extent_count=0;s->journal_start=s->journal_end=s->data_blocks=s->table_digest=0;for(i=0;i<OSP3_STORE_MAX_EXTENTS;i++)s->start[i]=s->end[i]=0;}
static int reject(struct Osp3StoreState*s,osp3_store_u32 r){clear(s);s->reason=r;return 0;}
static int overlap(osp3_store_u32 a,osp3_store_u32 ae,osp3_store_u32 b,osp3_store_u32 be){return a<be&&b<ae;}
int osp3_raw_storage_admit(const osp3_store_u8*p,osp3_store_u64 size,
                           osp3_store_u32 replay_floor,struct Osp3StoreState*s){
    osp3_store_u32 total,blocks,jstart,jcount,seq,count,table_end,i;
    clear(s);
    if(size<STORE_HEADER)return reject(s,OSP3_STORE_SHORT);
    if(r64(p)!=STORE_MAGIC)return reject(s,OSP3_STORE_IDENT);
    if(r32(p+8)!=1U)return reject(s,OSP3_STORE_VERSION);
    total=r32(p+16);blocks=r32(p+24);jstart=r32(p+28);jcount=r32(p+32);
    seq=r32(p+36);count=r32(p+40);
    if(r32(p+12)!=STORE_HEADER||total<STORE_HEADER||total>size||
       r32(p+20)!=4096U||blocks<8U||jstart<2U||jstart>=blocks||
       jcount==0U||jcount>blocks-jstart||count==0U||
       count>OSP3_STORE_MAX_EXTENTS||count>(0xffffffffU-STORE_HEADER)/STORE_EXTENT||
       r32(p+44)!=0U||r32(p+56)!=0U||r32(p+60)!=0U)
        return reject(s,OSP3_STORE_GEOMETRY);
    table_end=STORE_HEADER+count*STORE_EXTENT;
    if(table_end>total)return reject(s,OSP3_STORE_GEOMETRY);
    if(fnv(p,52)!=r32(p+52))return reject(s,OSP3_STORE_HEADER_CHECKSUM);
    if(fnv(p+STORE_HEADER,total-STORE_HEADER)!=r32(p+48))
        return reject(s,OSP3_STORE_TABLE_CHECKSUM);
    if(seq==0U||seq<=replay_floor)return reject(s,OSP3_STORE_REPLAY);
    s->sequence=seq;s->extent_count=count;s->journal_start=jstart;
    s->journal_end=jstart+jcount;s->table_digest=r32(p+48);
    for(i=0;i<count;i++){
        const osp3_store_u8*e=p+STORE_HEADER+i*STORE_EXTENT;
        osp3_store_u32 start=r32(e),n=r32(e+4),kind=r32(e+8),flags=r32(e+12),k;
        if(n==0U||start<2U||start>=blocks||n>blocks-start||
           kind==0U||kind>3U||flags>1U)
            return reject(s,OSP3_STORE_EXTENT_RANGE);
        s->start[i]=start;s->end[i]=start+n;s->data_blocks+=n;
        if(overlap(start,start+n,jstart,jstart+jcount))
            return reject(s,OSP3_STORE_JOURNAL_OVERLAP);
        for(k=0;k<i;k++)if(overlap(start,start+n,s->start[k],s->end[k]))
            return reject(s,OSP3_STORE_EXTENT_OVERLAP);
    }
    s->accepted=1;s->reason=OSP3_STORE_ACCEPT;return 1;
}
