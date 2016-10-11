
#include <stdlib.h>

#include "NLFLib.h"
#include "Mutex.h"
#include "ConditionVariable.h"

using namespace NOMADSUtil;

int main (int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf (stderr, "usage: %s <Millisec>\n",argv[0]);
        return -1;
    }
    int64 i64StartTime, i64EndTime;
    int32 i32Wait = atoi (argv[1]);
    Mutex m;
    ConditionVariable cv (&m);

    i64StartTime = getTimeInMilliseconds();
    for (int32 i32count = 0; i32count < 100; i32count++) {
        m.lock();
        cv.wait (i32Wait);
        m.unlock();
    }
    
    i64EndTime = getTimeInMilliseconds();
    printf("Avarage TIME for 100 wait (%d): %.2f ms\n", i32Wait, ((float)(i64EndTime - i64StartTime))/100);
    return 0;
}

