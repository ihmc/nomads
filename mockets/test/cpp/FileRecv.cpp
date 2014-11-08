#include "Mocket.h"
#include "ServerMocket.h"
#include "Socket.h"
#include "TCPSocket.h"

#include "NLFLib.h"
#include "Logger.h"

#include <stdio.h>

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
        _CrtSetBreakAlloc (54);
    #endif*/

    pLogger = new Logger();
    #if defined (_DEBUG)
        pLogger->initLogFile ("filerecv.log", false);
        pLogger->enableFileOutput();
        pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    #endif
    pLogger->disableScreenOutput();

    if ((argc != 2) && (argc != 3)) {
        fprintf (stderr, "usage: %s <filetowrite> [-mockets | -sockets]\n", argv[0]);
        return -1;
    }

    bool _useMockets = true;

    FILE *file = fopen (argv[1], "wb");
    if (file == NULL) {
        fprintf (stderr, "failed to create file %s\n", argv[1]);
        return -2;
    }
    if (argc == 3) {
        if (0 == strcmp (argv[2], "-mockets")) {
            _useMockets = true;
        }
        else if (0 == strcmp (argv[2], "-sockets")) {
            _useMockets = false;
        }
    }
    
    if (_useMockets) {
        ServerMocket servMocket;
        servMocket.listen (REM_PORT);
        Mocket *pMocket = servMocket.accept();

        if (pMocket) {
            int iBytesRead = 0;
            int i;
            char buf [1024*100]; //1024
		    int iCount = 0;
            pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            #if defined (_DEBUG)
                FILE *fileTimeLog = fopen ("filerecvtime.log", "w");
            #endif
            int64 i64StartTime = getTimeInMilliseconds();
            while (true) {
                if ((i = pMocket->receive (buf, sizeof (buf))) > 0) {
                    //printf ("-->>Received:: [%d]\n", i);
                    fwrite (buf, 1, i, file);
                    iBytesRead += i;
                    #if defined (_DEBUG)
                        fprintf (fileTimeLog, "progress, %lu, %d, %d\n", (uint32) (getTimeInMilliseconds()-i64StartTime), i, iBytesRead);
                    #endif
                }
                else {
                    printf ("receive returned %d; closing connection\n", i);
                    printf ("elapsed time = %llu\n", getTimeInMilliseconds() - i64StartTime);
                    break;
                }
            }
            fclose (file);
            #if defined (_DEBUG)
                fclose (fileTimeLog);
            #endif
            pMocket->close();

            int iTime = (int) (getTimeInMilliseconds() - i64StartTime);
            if (iTime == 0) {
                iTime = 1;
            }
            printf ("Time to receive %d bytes = %d (%d) bytes/sec\n", iBytesRead, iTime, (iBytesRead*1000)/iTime);
            printf ("Mocket statistics:\n");
            printf ("    Packets sent: %d\n", pMocket->getStatistics()->getSentPacketCount());
            printf ("    Bytes sent: %d\n", pMocket->getStatistics()->getSentByteCount());
            printf ("    Packets received: %d\n", pMocket->getStatistics()->getReceivedPacketCount());
            printf ("    Bytes received: %d\n", pMocket->getStatistics()->getReceivedByteCount());
            printf ("    Retransmits: %d\n", pMocket->getStatistics()->getRetransmitCount());
    //        printf ("    Packets discarded - below window threshold: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iBelowWindow);
    //        printf ("    Packets discarded - no room: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iNoRoom);
    //        printf ("    Packets discarded - overlap: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iOverlap);

            delete pMocket;
        }
    }

    else {
        TCPSocket *_pServerSocket = new TCPSocket();
        _pServerSocket->setupToReceive (REM_PORT);

        int iBytesRead = 0;
        char buf [1024*100];
        int i;
        int iTime = 0;

        Socket *pSocket = _pServerSocket->accept();
        if (pSocket) {
            int64 i64StartTime = getTimeInMilliseconds();
            while (true) {
                if ((i = pSocket->receive (buf, sizeof (buf))) > 0) {
                    fwrite (buf, 1, i, file);
                    iBytesRead += i;
                }
                else {
                    printf ("Socket receive returned %d; closing connection\n", i);
                    break;
                }
            }
            pSocket->disconnect();
            fclose (file);

            iTime = (int) (getTimeInMilliseconds() - i64StartTime);
            if (iTime == 0) {
                iTime = 1;
            }

            printf ("Sockets:Time to receive %d bytes = %d (%d) bytes/sec\n", iBytesRead, iTime, (iBytesRead*1000)/iTime);
        }
        delete pSocket;
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
