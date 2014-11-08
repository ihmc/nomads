/*
 * Client/server test the client send packets of MSG_DIMENSION at full speed
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
        
#define MSG_FREQUENCY 25
#define MSG_DIMENSION 1024
#define RELIABLE true
#define SEQUENCED true

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
bool reachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

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
    char buf[MSG_DIMENSION];

    int iBytesRead = 0;
    bool cont = true;
    int iter = 0;
    int iTotBytes = 0;

    printf ("ConnectionHandler: client handler thread started\n");

    _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    _pMocket->registerPeerReachableCallback (reachablePeerCallback, NULL);

    uint64 ui64StartTime = NOMADSUtil::getTimeInMilliseconds();
    uint64 ui64EndTime = 0;
    while (cont)  {
        iBytesRead = _pMocket->receive (buf, sizeof (buf));
        if (iBytesRead > 0) {
            printf ("DataRecv: received %d bytes. Iteration: %d\n", iBytesRead, iter);
            iTotBytes += iBytesRead;
            ui64EndTime = NOMADSUtil::getTimeInMilliseconds();
            iter ++;
        }
        else {
            cont = false;
            uint64 ui64TotTime = ui64EndTime - ui64StartTime;
            printf ("Done receiving. Received %d bytes in %llu time = %d bytes/sec\n", iTotBytes, ui64TotTime, iTotBytes/ui64TotTime*1000);
        }
    }

    printf("closing the mocket connection\n");

    _pMocket->close();

    delete _pMocket;
    
    printf ("ConnectionHandler: client handler thread finished\n");
}

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, unsigned short usIter, Statistics *pStats)
{
    int rc;
    static char buf [MSG_DIMENSION];
    uint32 ui32;

    int numIterations = (int) usIter;

    Mocket *pMocket = new Mocket();
    pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    pMocket->registerPeerReachableCallback (reachablePeerCallback, NULL);

    if (0 != (rc = pMocket->connect (pszRemoteHost, usRemotePort))) {
        fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                 pszRemoteHost, usRemotePort, rc);
        printf ("doClientTask: Unable to connect\n");
        delete pMocket;
        return -1;
    }

    // Reliable sequenced service
    MessageSender sender = pMocket->getSender (RELIABLE, SEQUENCED);
    uint64 ui64BeforeSend;
    uint32 ui32SleptFor;
    for (ui32 = 0; ui32 < numIterations; ui32++) {
        printf("Client iteration %d\n", ui32);
        // Send
        ui64BeforeSend = NOMADSUtil::getTimeInMilliseconds();
        sender.send (buf, sizeof (buf));
        //printf("**** time to send: %llu\n",getTimeInMilliseconds()-ui64BeforeSend);
        ui32SleptFor = (uint32)(NOMADSUtil::getTimeInMilliseconds()-ui64BeforeSend); // how much it has to sleep minus how much it slept
        if (ui32SleptFor < MSG_FREQUENCY) {
            //printf ("**** SLEEP %llu****\n",MSG_FREQUENCY-ui32SleptFor);
            sleepForMilliseconds(MSG_FREQUENCY-ui32SleptFor);
        }
    }

    printf("closing the mocket\n");
    pMocket->close();
    //!!// I need this sleep otherwise the program ends!! Why?
    sleepForMilliseconds(10000);

    delete pMocket;

    return 0;
}

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

    if ((argc != 3) && (argc != 4) && (argc != 5)) {
        fprintf (stderr, "usage: %s <server|client> <port> <remotehost> [<iterations>]\n", argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            pLogger = new Logger();
            pLogger->initLogFile ("DataSendReceiveServer.log", false);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            unsigned short usPort = atoi (argv[2]);
            
            printf ("Creating a ServerMocket on port %d\n", (int) usPort);
            ServerMocketThread *pSMT = new ServerMocketThread (usPort);
            pSMT->run();
        }
        else if (0 == stricmp (argv[1], "client")) {
            pLogger = new Logger();
            pLogger->initLogFile ("DataSendReceiveClient.log", false);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
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
/*    if (ulMilliSec > 10000) {
        return true;
    }
    else {
        return false;
    }
 */
    return false;
}

bool reachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    printf ("peer reachable again!! Unreachability interval: %lu ms\n", ulMilliSec);
    return true;
}
