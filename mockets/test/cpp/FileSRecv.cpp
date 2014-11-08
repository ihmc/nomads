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
    Mocket *pMocket = servMocket.accept();

    if (pMocket) {
        int iBytesRead = 0;
        int64 i64StartTime = getTimeInMilliseconds();
        int i;
		char buf1 [2048];
		char buf2 [2048];
		char buf3 [2048];
		uint32 ibuf1 = 50;  //450
		uint32 ibuf2 = 150;  //650
		uint32 ibuf3 = 2048; //1000*/
		int iCount = 0;
        pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
        while (true) {
            if ((i = pMocket->sreceive (-1, buf1, ibuf1, buf2, ibuf2, buf3, ibuf3, NULL)) > 0) {               
				//printf ("-->>Received:: [%d]\n", i);
                int iBytesToWrite = ibuf1;
                if (iBytesToWrite > i) {
                    iBytesToWrite = i;
                }
				fwrite (buf1, 1, iBytesToWrite, file);
                i -= iBytesToWrite;
			    //printf ("written on Buf1:: [%d]\n", iBytesToWrite);

                iBytesToWrite = ibuf2;
                if (iBytesToWrite > i) {
                    iBytesToWrite = i;
                }
				fwrite (buf2, 1, iBytesToWrite, file);
                i -= iBytesToWrite;
			    //printf ("written on Buf2:: [%d]\n", iBytesToWrite);

                iBytesToWrite = ibuf3;
                if (iBytesToWrite > i) {
                    iBytesToWrite = i;
                }
				fwrite (buf3, 1, iBytesToWrite, file);
			    //printf ("written on Buf3:: [%d]\n", iBytesToWrite);
            }
            else {
                printf ("receive returned %d; closing connection\n", i);
                break;
            }
        }
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
//        printf ("    Packets discarded - below window threshold: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iBelowWindow);
//        printf ("    Packets discarded - no room: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iNoRoom);
//        printf ("    Packets discarded - overlap: %d\n", pMocket->getStatistics()->getDiscardedPacketCounts()._iOverlap);

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
