
#include <stdlib.h>
#include <stdio.h>

#include "GeoUtils.h"

using namespace NOMADSUtil;

int main (int argc, const char *argv[])
{
    if (argc < 5) {
        fprintf (stderr, "usage: %s <latitude1> <longitude1> <latitude2> <longitude2>\n", argv[0]);
        return -1;
    }

    float lat1 = (float) atof (argv[1]);
    float lon1 = (float) atof (argv[2]);
    float lat2 = (float) atof (argv[3]);
    float lon2 = (float) atof (argv[4]);

    printf ("The great circle distance between the points (%f, %f) and (%f, %f) is %f meters.\n",
            lat1, lon1, lat2, lon2, greatCircleDistance (lat1, lon1, lat2, lon2));

    return 0;
}

