#include "EventNotifier.h"

#include <stdio.h>
#include <string.h>

#include "NLFLib.h"

int main (int argc, char **argv)
{
    EventNotifier en ("ComponentName", "InstanceName");
    en.init (5000);
    
    char log[256];
    int i = 0;
    while (true) {
        sprintf (log, "log# %d", i++);
        printf ("Logging event: %s\n", log);
        en.logEvent (i, log, strlen(log)+1);
        sleepForMilliseconds (1000);
    }

    //never reached
    return 0;
}

