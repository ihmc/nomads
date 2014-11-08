#include "Mocket.h"
#include "MessageSender.h"
#include "ServerMocket.h"

#include "Logger.h"

#include <stdio.h>

#if defined (UNIX)
    #define stricmp strcasecmp
    #include <strings.h>
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif    

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
int handleConnectionFromClient (Mocket *pMocket);
int handleConnectionToServer (Mocket *pMocket);

int main (int argc, char *argv[])
{
	#if defined (WIN32) && defined (_DEBUG)
        getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (53);
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
            pLogger->initLogFile ("DeleteMessageServer.log", false);
            pLogger->enableFileOutput();
            unsigned short usPort = atoi (argv[2]);
            ServerMocket serverMocket;
            serverMocket.listen (usPort);
            Mocket *pMocket = serverMocket.accept();
            if (pMocket == NULL) {
                fprintf (stderr, "%s: failed to accept a connection\n", argv[0]);
                return -2;
            }
            pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            handleConnectionFromClient (pMocket);
            delete pMocket;
            serverMocket.close();
        }
        else if (0 == stricmp (argv[1], "client")) {
            pLogger->initLogFile ("DeleteMessageClient.log", false);
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
    printf ("Received connection from client - waiting for data\n");
    uint32 ui32;
    do {
        pMocket->receive (&ui32, sizeof (uint32));
        printf ("%lu\n", ui32);
    }
    while (ui32 != 100);
    printf ("Received 100 from client - sending reply 0\n");
    MessageSender sender = pMocket->getSender (true, true);
    ui32 = 0;
    sender.send (&ui32, sizeof (uint32));
    printf ("Closing connection\n");
    pMocket->close();
    return 0;
}

int handleConnectionToServer (Mocket *pMocket)
{
    int rc;
    uint32 ui32;
    printf ("Connected to server - sending 10 messages\n");
    MessageSender sender = pMocket->getSender (true, true);
    for (ui32 = 0; ui32 < 10; ui32++) {
        sender.send (&ui32, sizeof (uint32));
    }
    printf ("Sleeping for 10 seconds...\n");
    sleepForMilliseconds (10000);
    printf ("Disconnect the network and press return\n");
    getchar();
    printf ("Now sending numbers 10 through 99 with a tag of 1 for odd and tag of 2 for even numbers\n");
    for (ui32 = 10; ui32 < 100; ui32++) {
        sender.send (&ui32, sizeof (uint32), (ui32 % 2) ? 1 : 2, 0);
    }
    printf ("Sent numbers, sleeping for 10 seconds...\n");
    sleepForMilliseconds (10000);
    printf ("Deleting all packets with tag 1\n");
    rc = pMocket->cancel (true, true, 1);
    printf ("Mocket::cancel() returned %d\n", rc);
    printf ("Reconnect network now\n");
    ui32 = 100;
    sender.send (&ui32, sizeof (uint32));
    printf ("Sent 100, waiting for reply\n");
    pMocket->receive (&ui32, sizeof (uint32));
    printf ("Received %lu; closing connection\n", ui32);
    pMocket->close();
    return 0;
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    // Don't do anything with the peer unreachable warning in this testcase
    printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    return false;
}
