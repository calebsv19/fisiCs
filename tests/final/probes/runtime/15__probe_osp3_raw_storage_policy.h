// SPDX-License-Identifier: Apache-2.0
#ifndef OSP3_RAW_STORAGE_POLICY_H
#define OSP3_RAW_STORAGE_POLICY_H
typedef unsigned char osp3_store_u8;
typedef unsigned int osp3_store_u32;
typedef unsigned long osp3_store_u64;
#define OSP3_STORE_MAX_EXTENTS 4U
enum Osp3StoreReason {
    OSP3_STORE_ACCEPT=0, OSP3_STORE_SHORT=1, OSP3_STORE_IDENT=2,
    OSP3_STORE_VERSION=3, OSP3_STORE_GEOMETRY=4,
    OSP3_STORE_HEADER_CHECKSUM=5, OSP3_STORE_TABLE_CHECKSUM=6,
    OSP3_STORE_REPLAY=7, OSP3_STORE_EXTENT_RANGE=8,
    OSP3_STORE_JOURNAL_OVERLAP=9, OSP3_STORE_EXTENT_OVERLAP=10
};
struct Osp3StoreState {
    osp3_store_u32 accepted, reason, sequence, extent_count;
    osp3_store_u32 journal_start, journal_end, data_blocks, table_digest;
    osp3_store_u32 start[OSP3_STORE_MAX_EXTENTS], end[OSP3_STORE_MAX_EXTENTS];
};
int osp3_raw_storage_admit(const osp3_store_u8*, osp3_store_u64,
                           osp3_store_u32, struct Osp3StoreState*);
#endif
