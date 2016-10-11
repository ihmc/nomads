
#include <stdlib.h>

#include "NLFLib.h"

using namespace NOMADSUtil;

int main (int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf (stderr, "usage: %s <Millisec>\n",argv[0]);
        return -1;
    }
    int64 i64StartTime, i64EndTime;
    int32 i32Sleep = atoi (argv[1]);

    i64StartTime = getTimeInMilliseconds();
    for (int32 i32count = 0; i32count < 100; i32count++) {
        sleepForMilliseconds (i32Sleep);
    }
    i64EndTime = getTimeInMilliseconds();

    printf("Avarage TIME for 100 sleepForMilliseconds (%d): %.2f ms\n", i32Sleep, ((float)(i64EndTime - i64StartTime))/100);
    return 0;
}

