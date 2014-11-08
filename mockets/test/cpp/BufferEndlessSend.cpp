#include "Mocket.h"
#include "MessageSender.h"

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
    #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (58);
    #endif

    pLogger = new Logger();
    pLogger->initLogFile ("bufferendlessrecv.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);

    if (argc != 2) {
        fprintf (stderr, "usage: %s <hostname>\n", argv[0]);
        return -1;
    }

    int rc;
    Mocket *pm = new Mocket();
    pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    if (0 != (rc = pm->connect (argv[1], REM_PORT))) {
        fprintf (stderr, "failed to connect to host %s; rc = %d\n", argv[1], rc);
        delete pm;
        return -3;
    }

    char * buf = NULL;
    int i;
    int buffsize = 512;
    buf = (char*) malloc(buffsize);
    for (i = 0; i < buffsize; i++) {
        buf[i] = 'a';
    }
    int64 i64TStart = getTimeInMilliseconds();
    

    MessageSender sender = pm->getSender (true, true);
    while (getTimeInMilliseconds() <= (i64TStart + 60000)) {
        if (sender.send (buf, buffsize) < 0) {
            printf ("send failed with rc < 0\n");
            break;
        }
    }

    printf ("FileSend: sent data so closing the connection:\n");
    pm->close();

    printf ("Mocket statistics:\n");
    printf ("    Packets sent: %d\n", pm->getStatistics()->getSentPacketCount());
    printf ("    Bytes sent: %d\n", pm->getStatistics()->getSentByteCount());
    printf ("    Packets received: %d\n", pm->getStatistics()->getReceivedPacketCount());
    printf ("    Bytes received: %d\n", pm->getStatistics()->getReceivedByteCount());
    printf ("    Retransmits: %d\n", pm->getStatistics()->getRetransmitCount());

    delete pm;

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
