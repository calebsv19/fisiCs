#ifndef FISICS_PROBE_WAVE91_TYPEDEF_MEMBER_ARRAY_INDIRECT_CONTRACT_H
#define FISICS_PROBE_WAVE91_TYPEDEF_MEMBER_ARRAY_INDIRECT_CONTRACT_H

typedef struct Wave91Payload {
    long long lane[3];
    int stamp;
} Wave91Payload;

typedef int Wave91Samples[3];
typedef Wave91Payload Wave91Transform(Wave91Samples samples, Wave91Payload seed);
typedef Wave91Transform *Wave91TransformPtr;
typedef Wave91TransformPtr Wave91TransformArray[2];

typedef struct Wave91Dispatch {
    Wave91TransformArray transforms;
    int generation;
} Wave91Dispatch;

Wave91Dispatch wave91_make_dispatch(int reverse);

#endif
