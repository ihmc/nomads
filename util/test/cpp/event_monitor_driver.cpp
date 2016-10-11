#include "EventMonitor.h"

#include <stdio.h>
#include <string.h>

#include "NLFLib.h"

int main (int argc, char **argv)
{
    EventMonitor em;
    em.initReceiveSocket (5000);
    em.initFileOutput (NULL);
    em.go();

    return 0; //never reached
}

