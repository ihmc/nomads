/*
 *
 *
 *
 */

#include "Mocket.h"
#include "ServerMocket.h"

#include "NLFLib.h"

#define MSG_DIMENSION 1024
#define TAG_SENSOR_A 1
#define TAG_SENSOR_B 2

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

int main (int argc, char *argv[])
{
    if (argc != 2) {
        fprintf (stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }
    
    unsigned short usPort = atoi (argv[1]);

    printf ("Creating a ServerMocket on port %d\n", (int) usPort);
    ServerMocket *_pServerMocket = new ServerMocket();
    _pServerMocket->listen (usPort);
    Mocket *pMocket = _pServerMocket->accept();
    
    if (!pMocket) {
        fprintf (stderr, "Error creating mockets connection\n");
        return -1;
    }
    
    printf ("ServerMocket: got a connection\n");
    pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    
    char buf[MSG_DIMENSION];

    int32 i32StreamId, i32msgId;
    int64 i64msgTS, now, i64Time0A, i64TS0A, i64Time0B, i64TS0B;
    int delay, replaced, lastIdRcvA, lastIdRcvB;
    lastIdRcvA = 0;
    lastIdRcvB = 0;

    FILE *file1 = fopen ("server-mocket-msgreplaceTEST2-1.txt", "w");
    if (file1 == NULL) {
        fprintf (stderr, "failed to append to file server-mocket-msgreplaceTEST2.txt\n");
        return -1;
    }
    FILE *file2 = fopen ("server-mocket-msgreplaceTEST2-2.txt", "w");
    if (file2 == NULL) {
        fprintf (stderr, "failed to append to file server-mocket-msgreplaceTEST2.txt\n");
        return -1;
    }

    while (true)  {
        if (pMocket->receive (buf, sizeof (buf)) < 0) {
            printf ("Closing connection\n");
            break;
        }
        now = getTimeInMilliseconds();
        // Extract the application sequence number and the timestamp from the msg
        i32StreamId = ((int32*)buf)[0];
        if (i32StreamId == TAG_SENSOR_A) {
            i32msgId = ((int32*)buf)[4];
            i64msgTS = ((int64*)buf)[8];
            if (i32msgId == 0) {
                i64Time0A = now;
                i64TS0A = i64msgTS;
                lastIdRcvA = 0;
            }
            else {
                delay = (int)(now - i64Time0A - (i64msgTS - i64TS0A));
                replaced = i32msgId - lastIdRcvA - 1;
                lastIdRcvA = i32msgId;

                printf("MocketTypeA: time %lu\ttimestamp %lu\tmessageId %d\tdelay %d\treplaced %d\n", 
                                               (long)now, (long)i64msgTS, i32msgId, delay, replaced);
                fprintf(file1, "MocketTypeA: %lu %lu %d %d\n", 
                        (long)now, (long)i64msgTS, i32msgId, delay);
                fflush(file1);
            }
        }
        else if (i32StreamId == TAG_SENSOR_B) {
            i32msgId = ((int32*)buf)[4];
            i64msgTS = ((int64*)buf)[8];
            if (i32msgId == 0) {
                i64Time0B = now;
                i64TS0B = i64msgTS;
                lastIdRcvB = 0;
            }
            else {
                delay = (int)(now - i64Time0B - (i64msgTS - i64TS0B));
                replaced = i32msgId - lastIdRcvB - 1;
                lastIdRcvB = i32msgId;

                printf("MocketTypeB: time %lu\ttimestamp %lu\tmessageId %d\tdelay %d\treplaced %d\n", 
                                               (long)now, (long)i64msgTS, i32msgId, delay, replaced);
                fprintf(file2, "MocketTypeB: %lu %lu %d %d\n", 
                        (long)now, (long)i64msgTS, i32msgId, delay);
                fflush(file2);
            }
        }
        else {
            printf ("ERROR!!!!!\n");
        }
    }
       
    printf("closing the mocket connection\n");
    fclose (file1);
    fclose (file2);

    pMocket->close();

    delete pMocket;
    
    printf ("ConnectionHandler: client handler thread finished\n");
    return 0;
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 10000) {
        return true;
    }
    else {
        return false;
    }
}

