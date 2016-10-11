#include <stdio.h>

#include "RollingBoundedBitmap.h"

using namespace NOMADSUtil;

int main (int argc, char *argv[])
{
    RollingBoundedBitmap rmb (66);
    rmb.set (60);
    rmb.set (30);
    if (rmb.isSet (0)) {
        printf ("Bit 0 is set and it should not be\n");
    }
    else {
        printf ("Bit 0 is not set, which is correct\n");
    }
    rmb.display();
    rmb.set (100);
    if (rmb.isSet (0)) {
        printf ("Bit 0 is set, which is correct\n");
    }
    if (rmb.isSet (100)) {
        printf ("Bit 100 is set, which is correct\n");
    }
    rmb.display();
    rmb.set (4095);
    rmb.display();
    return 0;
}
