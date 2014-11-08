#include "Mocket.h"
#include "ServerMocket.h"

#include "NLFLib.h"
#include "Logger.h"

#include <stdio.h>
#include <sstream>

#define REM_PORT 4000

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif     

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

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
    pLogger->initLogFile ("recvcongestion.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    //pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    pLogger->setDebugLevel (Logger::L_HighDetailDebug);

    if (argc != 2) {
        fprintf (stderr, "usage: %s\n", argv[0]);
        return -1;
    }

    std::stringstream sss(argv[1]);
    int port;
  
    if( (sss >> port).fail() )
    {
        fprintf (stderr, "argument must be a numerical string \n");
        return -1;
    }

    ServerMocket servMocket;
    servMocket.listen (port);
    //Mocket *pMocket; //NINO
    //while(pMocket = servMocket.accept()) //NINO
    Mocket *pMocket = servMocket.accept();
    //{
        if (pMocket) {
            int iBytesRead = 0;
            int64 i64StartTime = getTimeInMilliseconds();
            int i;
            int buffsize = 102400;
            char buf [buffsize]; //1024
            int iCount = 0;
            pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            while (true) {
            	if (i = pMocket->receive (buf, buffsize) > 0) {
                    iBytesRead += i;
                }
                else {
                    printf ("receive returned %d; closing connection\n", i);
                    break;
                }
            }
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
//            printf ("    Packets discarded - below window threshold: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iBelowWindow);
//            printf ("    Packets discarded - no room: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iNoRoom);
//            printf ("    Packets discarded - overlap: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iOverlap);

            //delete pMocket;
        }    
    //}

    delete pMocket;
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
