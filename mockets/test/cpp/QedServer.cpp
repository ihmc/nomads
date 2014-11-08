/*
 *
 *
 *
 */

#include "Mocket.h"
#include "ServerMocket.h"

#include "NLFLib.h"

#define MSG_DIMENSION 1024

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

    int32 i32msgId;
    int64 i64msgTS, now, i64Time0, i64TS0;
    int delay, replaced, lastIdRcv;
    lastIdRcv = 0;

    FILE *file = fopen ("server-mocket-msgreplace.txt", "w");
    if (file == NULL) {
        fprintf (stderr, "failed to append to file server-mocket-msgreplace.txt\n");
        return -1;
    }

    while (true)  {
        if (pMocket->receive (buf, sizeof (buf)) < 0) {
            printf ("Closing connection\n");
            break;
        }
        now = getTimeInMilliseconds();
        // Extract the application sequence number and the timestamp from the msg
        i32msgId = ((int32*)buf)[0];
        i64msgTS = ((int64*)buf)[4];
        if (i32msgId == 0) {
            i64Time0 = now;
            i64TS0 = i64msgTS;
            lastIdRcv = 0;
        }
        else {
            delay = (int)(now - i64Time0 - (i64msgTS - i64TS0));
            /*if (delay < 0) {
                delay = 0;
            }*/
            replaced = i32msgId - lastIdRcv - 1;
            lastIdRcv = i32msgId;

            printf("Mocket Msg Recvd:: time %lu\ttimestamp %lu\tmessageId %d\tdelay %d\treplaced %d\n", 
                    (long)now, (long)i64msgTS, i32msgId, delay, replaced);
            fprintf(file, "Mocket: %lu %lu %d %d\n", 
                    (long)now, (long)i64msgTS, i32msgId, delay);
            fflush(file);
        }
    }
       
    printf("closing the mocket connection\n");
    fclose (file);

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

