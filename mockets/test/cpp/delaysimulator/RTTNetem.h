#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>


#include "wnetemu.h"

#define PRINT_DELAY 5000
#define RAMP_INCREASE_DELAY 15
#ifndef UINT_MAX
#define UINT_MAX    4294967295U

#define FOURTY_PERCENT ((UINT_MAX / 100) * 40)

typedef long long int int64;
typedef unsigned int uint32;


int64 getTimeInMilliseconds (void);
void netemStep (void);
void netemRamp (void);
void netemSlowRamp (void);
void netemSteps (void);
void writeLog (char * pLogMsg, int64 i64TimeStamp, uint32 ui32Delay);

FILE *flog;

#endif //UINT_MAX
