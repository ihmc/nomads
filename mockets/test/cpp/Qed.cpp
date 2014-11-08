/* Client/server test the client send packets of 1KB using the replacement function
 * The test is ment to be run in two different machines using NistNet to work on the delay, packet loss, and bandwidth
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "InetAddr.h"
#include "Logger.h"
#include "Mocket.h"
#include "MessageSender.h"
#include "NLFLib.h"
#include "ServerMocket.h"
#include "Socket.h"
#include "TCPSocket.h"
#include "Thread.h"

#include "Statistics.h"

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif

using namespace NOMADSUtil;        
        
#define MSG_FREQUENCY 98
#define MSG_DIMENSION 1024
#define RELIABLE true
#define SEQUENCED true

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

class ServerMocketThread : public Thread
{
    public:
        ServerMocketThread (int usServerPort);
        void run (void);

    private:
        ServerMocket *_pServerMocket;
};

class ConnHandler : public Thread
{
    public:
        ConnHandler (Mocket *pMocket);
        void run (void);

    private:
        Mocket *_pMocket;
};

ServerMocketThread::ServerMocketThread (int usServerPort)
{
    _pServerMocket = new ServerMocket();
    _pServerMocket->listen (usServerPort);
}

void ServerMocketThread::run (void)
{
    while (true) {
        Mocket *pMocket = _pServerMocket->accept();

        printf ("ServerMocket: got a connection\n");
        ConnHandler *pHandler = new ConnHandler (pMocket);
        pHandler->start();
    }

}

ConnHandler::ConnHandler (Mocket *pMocket)
{
    _pMocket = pMocket;
}

void ConnHandler::run (void)
{
    int i;
    int msgSize = 1024;
    char buf[1024];

    int iBytesRead = 0;
    int iBytesToRead = 0;
    int iTime = 0;
    bool cont = true;

    printf ("ConnectionHandler: client handler thread started\n");
    int32 i32msgId;

    int64 i64msgTS, now, i64Time0, i64TS0;
    int delay, replaced, lastIdRcv;
    lastIdRcv = 0;

    iBytesToRead = msgSize;

    _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    FILE *file = fopen ("server-mocket-msgreplace.txt", "a");
    if (file == NULL) {
        fprintf (stderr, "failed to append to file server-mocket-msgreplace.txt\n");
        return;
    }

    while (cont)  {
        while (iBytesRead < iBytesToRead) {
            if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
                    iBytesRead += i;
            }
            else {
                printf ("receive returned %d; closing connection\n", i);
                cont = false;
                break;
            }
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

            printf("Mocket Msg Recvd:: time -%lu-\ttimestamp %lu\tmessageId %d\tdelay %d\treplaced %d\n", 
                    (long)now, (long)i64msgTS, i32msgId, delay, replaced);
            fprintf(file, "Mocket Msg Recvd:: time %lu timestamp %lu %d %d\n", 
                    (long)now, (long)i64msgTS, i32msgId, delay);
            fflush(file);
        }
        iBytesRead = 0;
    }

    printf("closing the mocket connection\n");
    fclose (file);

    _pMocket->close();

    delete _pMocket;
    
    printf ("ConnectionHandler: client handler thread finished\n");
}

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, unsigned short usIter, Statistics *pStats)
{
    int rc;
    // Packet size 1Kb
    static char buf [MSG_DIMENSION];
    uint32 ui32;

    int numIterations = (int) usIter;

    Mocket *pMocket = new Mocket();
    pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    if (0 != (rc = pMocket->connect (pszRemoteHost, usRemotePort))) {
        fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                 pszRemoteHost, usRemotePort, rc);
        printf ("doClientTask: Unable to connect\n");
        delete pMocket;
        return -1;
    }

    // Reliable sequenced service
    MessageSender sender = pMocket->getSender (RELIABLE, SEQUENCED);
    for (ui32 = 0; ui32 < numIterations; ui32++) {
        printf("Client iteration %d\n", ui32);
        // Insert a sequence number in the msg
        ((int*)buf)[0] = ui32;
        // Insert the current time in the packet
        ((int64*)buf)[4] = getTimeInMilliseconds();
        // Send using the replace functionality
        sender.replace (buf, sizeof (buf), 1, 1);
        sleepForMilliseconds(MSG_FREQUENCY);
    }

    printf("closing the mocket\n");
    pMocket->close();
    //!!// I need this sleep otherwise the program ends!! Why?
    sleepForMilliseconds(10000);

    return 0;
}

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    pLogger->initLogFile ("qed.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);

    #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (58);
    #endif

    if ((argc != 3) && (argc != 4) && (argc != 5)) {
        fprintf (stderr, "usage: %s <server|client> <port> <remotehost> [<iterations>]\n", argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            unsigned short usPort = atoi (argv[2]);
            
            printf ("Creating a ServerMocket on port %d\n", (int) usPort);
            ServerMocketThread *pSMT = new ServerMocketThread (usPort);
            pSMT->run();
        }
        else if (0 == stricmp (argv[1], "client")) {
            unsigned short usRemotePort = atoi (argv[2]);
            unsigned short usIterations;
            if (argc == 5) {
                usIterations = atoi (argv[4]);
            }
            else {
                usIterations = 20;
            }
            const char *pszRemoteHost = argv[3];
            printf ("Client Creating a Mocket to host %s on port %d\n", pszRemoteHost, (int) usRemotePort);

            Statistics mocketStats;
            int rc;
            if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, usIterations, &mocketStats))) {
                fprintf (stderr, "main: doClientTask failed for mockets with rc = %d\n", rc);
                return -3;
            }

        }
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
