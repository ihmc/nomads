#include "MocketStatusMonitor.h"

#include "Mocket.h"

#include <stdio.h>

int main (int argc, char *argv[])
{
    int rc;
    MocketStatusMonitor msm;
    FILE *fileLog;
    
    if (argc > 1 && argc != 3) {
    	printf ("Usage: ./MocketStatusMonitor <ip> <port>\n");
    	exit (1);
    }
    
    if (NULL == (fileLog = fopen ("mocket_stats.log", "w"))) {
        fprintf (stderr, "failed to open file mocket_stats.log for writing\n");
        return -1;
    }
    if (0 != (rc = msm.initReceiveSocket (Mocket::DEFAULT_STATS_PORT))) {
        fprintf (stderr, "failed to initialize MocketStatusMonitor; rc = %d\n", rc);
        fclose (fileLog);
        return -2;
    }
    msm.initFileOutput (fileLog);
    if (argc == 3) {
    	msm.initRelaying (argv[1], atoi(argv[2]));
    }

    msm.run();
    fclose (fileLog);
    return 0;
}
