#include "Mocket.h"
#include "MessageSender.h"
#include "Socket.h"
#include "TCPSocket.h"

#include <stdio.h>

#include "Logger.h"

#define REM_PORT 4000

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif       

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

int main (int argc, char *argv[])
{
    /*#if defined (WIN32) && defined (_DEBUG)
        getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        _CrtSetBreakAlloc (58);
    #endif*/

    pLogger = new Logger();
    #if defined (_DEBUG)
        pLogger->initLogFile ("filesend.log", false);
        pLogger->enableFileOutput();
        pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    #endif
    pLogger->disableScreenOutput();

    if ((argc != 3) && (argc != 4)) {
        fprintf (stderr, "usage: %s <filetosend> <hostname> [-mockets | -sockets]\n", argv[0]);
        return -1;
    }

    bool _useMockets = true;
    FILE *file = fopen (argv[1], "rb");
    if (file == NULL) {
        fprintf (stderr, "failed to open file %s\n", argv[1]);
        return -2;
    }
    if (argc == 4) {
        if (0 == strcmp (argv[3], "-mockets")) {
            _useMockets = true;
        }
        else if (0 == strcmp (argv[3], "-sockets")) {
            _useMockets = false;
        }
    }

    int rc;
    if (_useMockets) {
        Mocket *pm = new Mocket();
        pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
        if (0 != (rc = pm->connect (argv[2], REM_PORT))) {
            fprintf (stderr, "failed to connect to host %s; rc = %d\n", argv[2], rc);
            delete pm;
            return -3;
        }

        //char buf[512];
        char * buf = NULL;
        int n;
        int buffsize = 1024;

        MessageSender sender = pm->getSender (true, true);
        while (true) {
            if (buffsize > 2048) {
                buffsize /= 5;
            }
            buf = (char*) malloc(buffsize);

            if ((n = fread (buf, 1, buffsize, file)) <= 0) {
                printf ("%s: EOF reached so closing connection\n", argv[0]);
                free (buf);
                break;
            }
            else {
                //printf("using buffer of size %d. Sending %d bytes\n", buffsize, n);
                sender.send (buf, n);
            }

            free(buf);
            buffsize *= 1.25;
        }

        printf ("FileSend: sent data so closing the connection:\n");
        pm->close();
        fclose (file);

        printf ("Mocket statistics:\n");
        printf ("    Packets sent: %d\n", pm->getStatistics()->getSentPacketCount());
        printf ("    Bytes sent: %d\n", pm->getStatistics()->getSentByteCount());
        printf ("    Packets received: %d\n", pm->getStatistics()->getReceivedPacketCount());
        printf ("    Bytes received: %d\n", pm->getStatistics()->getReceivedByteCount());
        printf ("    Retransmits: %d\n", pm->getStatistics()->getRetransmitCount());
    //    printf ("    Packets discarded - below window threshold: %d\n", pm->getStatistics()->getDiscardedPacketCounts()._iBelowWindow);
    //    printf ("    Packets discarded - no room: %d\n", pm->getStatistics()->getDiscardedPacketCounts()._iNoRoom);
    //    printf ("    Packets discarded - overlap: %d\n", pm->getStatistics()->getDiscardedPacketCounts()._iOverlap);

        delete pm;
    }
    else {
        TCPSocket socket;
        if (0 != (rc = socket.connect (argv[2], REM_PORT))) {
            fprintf (stderr, "FileSend: failed to connect using sockets to remote host %s on port %d; rc = %d\n",
                     argv[2], REM_PORT, rc);
            return -1;
        }

        char * buf = NULL;
        int n;
        int buffsize = 1024;

        int64 i64StartTime = getTimeInMilliseconds();
        while (true) {
            if (buffsize > 2048) {
                buffsize /= 5;
            }
            buf = (char*) malloc(buffsize);

           if ((n = fread (buf, 1, buffsize, file)) <= 0) {
                printf ("%s: EOF reached so closing connection\n", argv[0]);
                free (buf);
                break;
            }
            else {
                //printf("using buffer of size %d. Sending %d bytes\n", buffsize, n);
                socket.send (buf, n);
            }

            free(buf);
            buffsize *= 1.25;
        }
        printf ("FileSend:Sockets: sent data so closing the connection\n");
        fclose (file);
        socket.disconnect();
    }

    
    delete pLogger;
    pLogger = NULL;

    #if defined (WIN32) && defined (_DEBUG)
        _CrtDumpMemoryLeaks();
    #endif

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
