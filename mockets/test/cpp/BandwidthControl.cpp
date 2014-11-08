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
#include "delaysimulator/RTTNetem.h"

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif

using namespace NOMADSUtil;

#define DEFAULT_MSG_FREQUENCY 25
#define DEFAULT_MSG_DIMENSION 1024
#define RELIABLE true
#define SEQUENCED true
#define DEFAULT_MAX_BANDWIDTH 15360
#define DEFAULT_ITERATIONS 1000

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
    char buf[DEFAULT_MSG_DIMENSION];

    int iBytesRead = 0;
    bool cont = true;
    int iter = 0;
    int iTotBytes = 0;

    printf ("ConnectionHandler: client handler thread started\n");

    _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

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

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, unsigned short usIter,
        uint32 ui32BandwidthLimit, uint32 ui32MsgDim, uint32 ui32MsgFrequency, Statistics *pStats)
{
    int rc;
    const int msg_dim = ui32MsgDim;
    static char buf [DEFAULT_MSG_DIMENSION];
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
    int r = pMocket->setMaxBandwidthLimit (ui32BandwidthLimit);
    printf ("setMaxBandwidthLimit returned %d\n", r);
    uint64 ui64BeforeSend;
    uint64 ui64StartTime= NOMADSUtil::getTimeInMilliseconds();
    uint32 ui32SleptFor;
    for (ui32 = 0; ui32 < numIterations; ui32++) {
        printf("Client iteration %d\n", ui32);
        // Send
        ui64BeforeSend = NOMADSUtil::getTimeInMilliseconds();
        sender.send (buf, sizeof (buf));
        //printf("**** time to send: %llu\n",NOMADSUtil::getTimeInMilliseconds()-ui64BeforeSend);
        ui32SleptFor = (uint32)(NOMADSUtil::getTimeInMilliseconds()-ui64BeforeSend); // how much it has to sleep minus how much it slept
        if (ui32SleptFor < ui32MsgFrequency) {
            //printf ("**** SLEEP %llu****\n",ui32MsgFrequency-ui32SleptFor);
            sleepForMilliseconds(ui32MsgFrequency-ui32SleptFor);
        }
    }
    int iTotBytes = numIterations*ui32MsgDim;
    uint64 ui64TotTime = NOMADSUtil::getTimeInMilliseconds() - ui64StartTime;
    printf ("Done sending. Sent %d bytes in %llu time = %d bytes/sec\n", iTotBytes, ui64TotTime, iTotBytes/ui64TotTime*1000);
    printf("Closing the mocket\n");
    pMocket->close();
    //!!// I need this sleep otherwise the program ends!! Why?
    sleepForMilliseconds(10000);

    return 0;
}

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    pLogger->initLogFile ("BandwidthControl.log", false);
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

    //if ((argc != 3) && (argc != 4) && (argc != 5)) {
    if (argc < 3) {
        fprintf (stderr, "usage: \n%s <server> <port>\n or\n"
                "%s <client> <port> <remotehost> [i iterations] b bandwidth_limit] [d msg_dimension] [f msg_frequency]\n", argv[0], argv[0]);
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
            const char *pszRemoteHost = argv[3];
            // Assign default values
            unsigned short usIterations = DEFAULT_ITERATIONS;
            uint32 ui32BandwidthLimit = DEFAULT_MAX_BANDWIDTH;
            uint32 ui32MsgDim = DEFAULT_MSG_DIMENSION;
            uint32 ui32MsgFrequency = DEFAULT_MSG_FREQUENCY;
            // Check if we got values different from default ones.
            unsigned short processedUntil = 4;
            while (argc > processedUntil) {
                char label = argv[processedUntil][0];
                switch (label)
                {
                    case 'i':
                        usIterations = atoi (argv[processedUntil+1]);
                        printf ("Got iterations: %d\n", usIterations);
                        break;
                    case 'b':
                        ui32BandwidthLimit = atoi (argv[processedUntil+1]);
                        printf ("Got bandwidth limit: %d\n", ui32BandwidthLimit);
                        break;
                    case 'd':
                        ui32MsgDim = atoi (argv[processedUntil+1]);
                        printf ("Got msg_dim: %d\n", ui32MsgDim);
                        break;
                    case 'f':
                        ui32MsgFrequency = atoi (argv[processedUntil+1]);
                        printf ("Got msg_frequency: %d\n", ui32MsgFrequency);
                        break;
                    default:
                        fprintf (stderr, "Error wrong input parameters!!\nUsage: \n%s <server> <port>\n or\n"
                                "%s <client> <port> <remotehost> [-i iterations] [-b bandwidth_limit] [-d msg_dimension] [-f msg_frequency]\n", argv[0], argv[0]);
                        return -1;
                }
                processedUntil = processedUntil+2;
            }

            printf ("Client Creating a Mocket to host %s on port %d\n",
                    pszRemoteHost, (int) usRemotePort);
            Statistics mocketStats;
            int rc;
            if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, usIterations, ui32BandwidthLimit, ui32MsgDim, ui32MsgFrequency, &mocketStats))) {
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

