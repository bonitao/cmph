#ifndef __MY_DEBUGC__
#define __MY_DEBUGC__

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif
#ifdef DEBUG
#define DEBUGP(args...) do { fprintf(stderr, "%s:%d ", __FILE__, __LINE__); fprintf(stderr, ## args); } while(0)
#else
#define DEBUGP(args...)
#endif

#endif
