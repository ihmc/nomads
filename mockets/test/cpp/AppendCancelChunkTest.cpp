#include "MessageSender.h"
#include "Mocket.h"
#include "ServerMocket.h"

#include "Logger.h"

#include <stdio.h>

#define NUM_MESSAGES 60

#if defined (UNIX)
    #define stricmp strcasecmp
    #include <strings.h>
#elif defined (WIN32)
    #define stricmp _stricmp

    #if defined (_DEBUG)
        #include <crtdbg.h>
    #endif
#endif

using namespace NOMADSUtil;
        
bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
int handleConnectionFromClient (Mocket *pMocket);
int handleConnectionToServer (Mocket *pMocket);

static int iDataSize;

int main (int argc, char *argv[])
{
    #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (223);
    #endif
    
	pLogger = new Logger();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    if (argc < 4) {
        fprintf (stderr, "usage: %s <server|client> <payload_size> <port> [<remotehost>]\n", argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            pLogger->initLogFile ("AppendCancelChunkServer.log", false);
            pLogger->enableFileOutput();
			iDataSize = atoi (argv[2]);
            unsigned short usPort = atoi (argv[3]);
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
            pLogger->initLogFile ("AppendCancelChunkClient.log", false);
            pLogger->enableFileOutput();
            if (argc < 5) {
                fprintf (stderr, "usage: %s client <payload_size> <port> <remotehost>\n", argv[0]);
                return -3;
            }

			iDataSize = atoi(argv[2]);
			unsigned short usRemotePort = atoi (argv[3]);
            const char *pszRemoteHost = argv[4];

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
	printf ("Received connection from client - waiting for data. (Buffer Size = %d)\n", iDataSize);
    uint32 ui32;

	char *pBuff = (char*) malloc(iDataSize);

    do {
        pMocket->receive (pBuff, iDataSize);
		memcpy (&ui32, pBuff, sizeof(uint32));
        printf ("%lu\n", ui32);
    }
    while (ui32 != NUM_MESSAGES);

    printf ("Received %d from client - sending reply 0\n", NUM_MESSAGES);
    MessageSender sender = pMocket->getSender (true, true);
    ui32 = 0;
    sender.send (&ui32, sizeof (uint32));
    printf ("Closing connection\n");
    pMocket->close();

	delete pBuff;
    return 0;
}

int handleConnectionToServer (Mocket *pMocket)
{
	printf("handleConnectionToServer. (Buffer Size = %d)\n", iDataSize);
	int rc;
    uint32 ui32;
	char *pBuffer = (char*) malloc(iDataSize);

	// initialize the buffer
	// (the first few bytes of the buffer will contain a msg seq number).
	for (int i = sizeof(uint32); i < iDataSize; i++) {
		pBuffer[i] = (char)(i%128);
	}

    printf ("Connected to server - sending 10 messages (0 through 9)\n");
    MessageSender sender = pMocket->getSender (true, true);
    for (ui32 = 0; ui32 < 10; ui32++) {
		memcpy (pBuffer, &ui32, sizeof(uint32));
        sender.send (pBuffer, iDataSize);
    }

    printf ("Sleeping for 5 seconds...\n");
    sleepForMilliseconds (5000);

    printf ("Disconnect the network and press return\n");
    getchar();
    
	printf ("Now sending messages 10 through %d with a tag of 1 for odd and tag of 2 for even message numbers\n", NUM_MESSAGES);
    for (ui32 = 10; ui32 < NUM_MESSAGES; ui32++) {
		memcpy (pBuffer, &ui32, sizeof(uint32));
        sender.send (pBuffer, iDataSize, (ui32 % 2) ? 1 : 2, 0);
		printf("Sent message with id %d\n", ui32);
    }

    printf ("Sent messages, sleeping for 5 seconds...\n");
    sleepForMilliseconds (5000);

    printf ("Deleting all packets with tag 1\n");
    rc = pMocket->cancel (true, true, 1);
    printf ("Mocket::cancel() returned %d\n", rc);

	printf ("Reconnect network now\n");
    
	ui32 = NUM_MESSAGES;
	//sending NUM_MESSAGES
	memcpy (pBuffer, &ui32, sizeof(uint32));
    sender.send (pBuffer, iDataSize);

    printf ("Sent 100, waiting for reply\n");
    pMocket->receive (&ui32, sizeof (uint32));
    printf ("Received %lu; closing connection\n", ui32);
    pMocket->close();

	delete pBuffer;
    return 0;
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    // Don't do anything with the peer unreachable warning in this testcase
    printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    return false;
}
