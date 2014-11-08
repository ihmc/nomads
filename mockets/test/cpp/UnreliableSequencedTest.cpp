#include "Mocket.h"
#include "MessageSender.h"
#include "ServerMocket.h"

#include "Logger.h"

#include <stdio.h>

#if defined (WIN32)
    #include <crtdbg.h>
    #define stricmp _stricmp
#elif defined (UNIX)
    #define stricmp strcasecmp
    #include <strings.h>
#endif

using namespace NOMADSUtil;
        
bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
int handleConnectionFromClient (Mocket *pMocket);
int handleConnectionToServer (Mocket *pMocket);

int main (int argc, char *argv[])
{
       #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (66);
    #endif


    pLogger = new Logger();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    if (argc < 2) {
        fprintf (stderr, "usage: %s <server|client> <port> [<remotehost>]\n", argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            pLogger->initLogFile ("unseqser.log", false);
            pLogger->enableFileOutput();
            unsigned short usPort = atoi (argv[2]);
            ServerMocket serverMocket;
            serverMocket.listen (usPort);
            Mocket *pMocket = serverMocket.accept();
            if (pMocket == NULL) {
                fprintf (stderr, "%s: failed to accept a connection\n", argv[0]);
                return -2;
            }
            fprintf (stdout, "received a connection\n");
            pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            handleConnectionFromClient (pMocket);
            delete pMocket;
            serverMocket.close();
        }
        else if (0 == stricmp (argv[1], "client")) {
            pLogger->initLogFile ("unseqcli.log", false);
            pLogger->enableFileOutput();
            if (argc < 3) {
                fprintf (stderr, "usage: %s client <port> <remotehost>\n", argv[0]);
                return -3;
            }
            unsigned short usRemotePort = atoi (argv[2]);
            const char *pszRemoteHost = argv[3];
            Mocket mocket;
            int rc;
            if (0 != (rc = mocket.connect (pszRemoteHost, usRemotePort))) {
                fprintf (stderr, "%s: failed to connect to %s:%d; rc = %d\n", argv[0], pszRemoteHost, (int)usRemotePort, rc);
                return -4;
            }
            fprintf (stdout, "connected; sending data\n");
            mocket.registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            handleConnectionToServer (&mocket);
            fprintf (stdout, "done sending data - sleeping for 60 seconds\n");
            sleepForMilliseconds (60000);
            mocket.close();
        }
    }

    delete pLogger;
    pLogger = NULL;

    #if defined (WIN32) && defined (_DEBUG)
        _CrtDumpMemoryLeaks();
    #endif

    return 0;
}

int handleConnectionFromClient (Mocket *pMocket)
{
    uint32 ui32NextMsg = 0;
    int64 i64TimeLastPacketReceived = 0;
    while (true) {
        uint32 ui32Msg;
        int rc = pMocket->receive (&ui32Msg, sizeof (uint32));
        int64 i64CurrTime = getTimeInMilliseconds();
        if (rc != sizeof (uint32)) {
            fprintf (stdout, "receive() returned %d; terminating\n", rc);
            return 0;
        }
        if (ui32NextMsg < ui32Msg) {
            fprintf (stdout, "lost packet: expected %lu but got %lu; elapsed time = %lu\n", ui32NextMsg, ui32Msg, (uint32) (i64CurrTime - i64TimeLastPacketReceived));
        }
        else if (ui32NextMsg > ui32Msg) {
            fprintf (stdout, "ERROR: out of sequence packet - expected %lu but got %lu\n", ui32NextMsg, ui32Msg);
        }
        ui32NextMsg = ui32Msg + 1;
        i64TimeLastPacketReceived = i64CurrTime;
    }
}

int handleConnectionToServer (Mocket *pMocket)
{
    MessageSender sender = pMocket->getSender (false, true);
    for (uint32 ui32Msg = 0; ui32Msg < 10000; ui32Msg++) {
        sender.send (&ui32Msg, sizeof (uint32));
    }
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
