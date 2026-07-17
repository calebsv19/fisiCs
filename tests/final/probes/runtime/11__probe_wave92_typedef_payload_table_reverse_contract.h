#ifndef FISICS_PROBE_WAVE92_TYPEDEF_PAYLOAD_TABLE_REVERSE_CONTRACT_H
#define FISICS_PROBE_WAVE92_TYPEDEF_PAYLOAD_TABLE_REVERSE_CONTRACT_H

typedef long long Wave92Lane[3];

typedef struct Wave92Payload {
    Wave92Lane lane;
    int stamp;
} Wave92Payload;

typedef Wave92Payload Wave92Transform(Wave92Payload seed,
                                      Wave92Lane delta,
                                      int bias);
typedef Wave92Transform *Wave92TransformPtr;
typedef Wave92TransformPtr Wave92TransformTable[2];

const Wave92TransformTable *wave92_get_table(int reverse);

#endif
