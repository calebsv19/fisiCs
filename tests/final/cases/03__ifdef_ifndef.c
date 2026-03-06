#define FEATURE_ON 1

#ifdef FEATURE_ON
int enabled = 1;
#else
int enabled = 2;
#endif

#ifndef FEATURE_OFF
int disabled = 3;
#else
int disabled = 4;
#endif
