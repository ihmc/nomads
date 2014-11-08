#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "NLFLib.h"
#include <unistd.h>
#include <string.h>

int main (int argc, char**argv){
    if (argc != 2) {
        printf ("Usage: %s <ip>\n", argv[0]);
        return (-1);
    }
    int delay = 20, phase = 1, loss = 5;
    char com[255];
    int64 tmodify;
    FILE *flog;
    flog = fopen ("nistnetlog.txt", "wb");
   
    system ("/etc/init.d/ntpd stop");
    sprintf (com, "ntpdate %s", argv[1]);
    system (com);
    system ("cnistnet -d");
    sprintf (com, "cnistnet -a %s 0.0.0.0 --drop %d --delay %d", argv[1], loss, delay);
    system (com);
    system ("cnistnet -u");
    tmodify = getTimeInMilliseconds();
    sprintf (com, "%llu\t%d\n", getTimeInMilliseconds(), delay);
    fwrite (com, sizeof (char), strlen (com), flog);
    
    while (delay >= 20 && delay <= 40) {
        usleep (10000);
        sprintf (com, "%llu\t%d\n", getTimeInMilliseconds(), delay);
        fwrite (com, sizeof (char), strlen (com), flog);
        if (getTimeInMilliseconds() >= (tmodify + 2000)) {
            if (delay == 40) {
                phase = -1;
            }
            delay += phase;
            sprintf (com, "cnistnet -a %s 0.0.0.0 --drop %d --delay %d", argv[1], loss, delay);
            system (com);
            system ("cnistnet -u");
            tmodify = getTimeInMilliseconds();
        }
    }
    system ("cnistnet -d");
    system ("/etc/init.d/ntpd start");
    fclose (flog);
    return 0;
}
