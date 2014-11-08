/* This test is supposed to be used with a client side (file MigrationFileSend)
 * This test is the server side to test node migration, suspend/resume process.
 */
#include "Mocket.h"
#include "ServerMocket.h"

#include "NLFLib.h"
#include "Logger.h"

#include <stdio.h>

#define REM_PORT 4000

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif     


using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
bool suspendReceivedCallback (void *pCallbackArg, unsigned long ulMilliSec);

int main (int argc, char *argv[])
{
    #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (54);
    #endif

    pLogger = new Logger();
    pLogger->initLogFile ("filerecv.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
	//printf ("-->>File to write: %s <filetowrite>\n", argv[0]);
    if (argc != 2) {
        fprintf (stderr, "usage: %s <filetowrite>\n", argv[0]);
        return -1;
    }

    FILE *file = fopen (argv[1], "wb");
    if (file == NULL) {
        fprintf (stderr, "failed to create file %s\n", argv[1]);
        return -2;
    }

    ServerMocket servMocket;
    servMocket.listen (REM_PORT);
    printf ("ServeMocket listening on port %d\n", REM_PORT);
    Mocket *pMocket = servMocket.accept();

    if (pMocket) {
        int iBytesRead = 0;
        int64 i64StartTime = getTimeInMilliseconds();
        int i;
        char buf [1024*100]; //1024
        pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
        pMocket->registerSuspendReceivedWarningCallback (suspendReceivedCallback, NULL);
        while (true) {
            if ((i = pMocket->receive (buf, sizeof (buf))) > 0) {
		//printf ("-->>Received:: [%d]\n", i);
                fwrite (buf, 1, i, file);
                iBytesRead += i;
		//printf ("read so far :: [%d]\n", iBytesRead);
            }
            else {
                printf ("receive returned %d; closing connection\n", i);
                break;
            }
        }
//        sleepForMilliseconds (5000);
        fclose (file);
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

        delete pMocket;
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

bool suspendReceivedCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    printf ("Warning: the mocket has been suspended from: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 60000) {
        printf ("closing connection after %lu ms\n", ulMilliSec);
        // The return value equal to true means that we are requesting to close the connection
        // Receiver takes care of closing it
        return true;
    }
    else {
        return false;
    }
}

