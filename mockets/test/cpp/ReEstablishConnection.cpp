/* This test is for the function reEstablishConn().
 * It re-establishes the connection from the same node, without transferring the state to a new node.
 * In the future a component will detect a change in the network attachment and automatically call reEstablishConn()
 * It can be specified what side to reEstablish or if we want to reEstablish both sides with the param -reEstablish
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

#define DATA_SIZE 1024

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
bool suspendReceivedCallback (void *pCallbackArg, unsigned long ulMilliSec);

class ServerMocketThread : public Thread
{
    public:
        ServerMocketThread (int usServerPort, bool bReEstablish);
        ~ServerMocketThread (void);
        void run (void);
        bool isRunning (void);

    private:
        ServerMocket *_pServerMocket;
        bool _isRunning;
        bool _bReEstablish;
};

class ConnHandler : public Thread
{
    public:
        ConnHandler (Mocket *pMocket, bool bReEstablish);
        ~ConnHandler (void);
        void run (void);
        bool isRunning (void);

    private:
        Mocket *_pMocket;
        bool _isRunning;
        bool _bReEstablish;
};

ServerMocketThread::ServerMocketThread (int usServerPort, bool bReEstablish)
{
    _pServerMocket = new ServerMocket();
    _pServerMocket->listen (usServerPort);
    _isRunning = false;
    _bReEstablish = bReEstablish;
}

void ServerMocketThread::run (void)
{
    _isRunning = true;
    Mocket *pMocket = _pServerMocket->accept();
    if (pMocket) {
        printf ("ServerMocket: got a connection\n");
        ConnHandler *pHandler = new ConnHandler (pMocket, _bReEstablish);
        pHandler->start();
		sleepForMilliseconds(1000);
        while (pHandler->isRunning()) {
            sleepForMilliseconds(1000);
        }
        delete pHandler;
        if (pMocket) {
            delete pMocket;
        }
        _isRunning = false;
    }
}

bool ServerMocketThread::isRunning (void)
{
    return _isRunning;
}

ServerMocketThread::~ServerMocketThread (void)
{
    delete _pServerMocket;
    _pServerMocket = NULL;
}

ConnHandler::ConnHandler (Mocket *pMocket, bool bReEstablish)
{
    _pMocket = pMocket;
    _isRunning = false;
    _bReEstablish = bReEstablish;
}

void ConnHandler::run (void)
{
    _isRunning = true;
    int i;
    char buf[1024];
    int iBytesRead = 0;
    int iBytesToRead = 0;
    int iTime = 0;
    char chReply = '.';
    bool bNewDataToRead = false;
    int iIterNumber = 0;

    printf ("ConnHandler::run: client handler thread started for an incoming mockets connection\n");
    _pMocket->setIdentifier ("SuspendResumeTest-Server");
    _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    _pMocket->registerSuspendReceivedWarningCallback (suspendReceivedCallback, NULL);

    // Receive something
    _pMocket->receive (&iBytesToRead, sizeof (iBytesToRead));
    printf ("ConnHandler::run: Received request to read packets of [%d] bytes from client...\n", iBytesToRead);
    bNewDataToRead = true;
    iBytesToRead = EndianHelper::ntohl ((uint32)iBytesToRead);
    // Send an acknowledgement
    MessageSender sender = _pMocket->getSender (true, true);
    printf ("ConnHandler::run: Sending acknowledgement...\n\n");
    sender.send (&chReply, sizeof (chReply));

    if (_bReEstablish) {
        // ReEstablishConnection on the server side
        int rcc;
        uint64 ui64StartReEstablish = getTimeInMilliseconds();
        rcc = _pMocket->reEstablishConn (20000);
        printf ("Mocket reEstablishConn finished with status %d\n", rcc);
        printf ("TIME FOR REESTABLISH **  %llu  **\n", (getTimeInMilliseconds() - ui64StartReEstablish));
        if (rcc != 0) {
            _pMocket->close();
            delete _pMocket;
            _isRunning = false;
            return;
        }
    }

    // Read data
    while (true) {
        if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
            printf ("ConnHandler::run: Received %d bytes\n", i);
        }
        else {
            printf ("ConnHandler::run: receive returned %d; no more data to read from client\n", i);
            break;
        }
    }
    _pMocket->close();

    // Save results to a file
    FILE *file = fopen ("stats-server-MsgMockets-cpp.txt", "a");
    if (file == NULL) {
        fprintf (stderr, "failed to append to file stats-MsgMockets-cpp.txt\n");
        return;
    }
    fprintf (file, "%lu, %d, %d, %d, %d, %d, %d, %d, %d\n", (unsigned long) (getTimeInMilliseconds()/1000), iTime,
             _pMocket->getStatistics()->getSentPacketCount(),
             _pMocket->getStatistics()->getSentByteCount(),
             _pMocket->getStatistics()->getReceivedPacketCount(),
             _pMocket->getStatistics()->getReceivedByteCount(),
             _pMocket->getStatistics()->getRetransmitCount(),
             _pMocket->getStatistics()->getDuplicatedDiscardedPacketCount(),
             _pMocket->getStatistics()->getNoRoomDiscardedPacketCount());

    fclose (file);
    delete _pMocket;
    printf ("ConnHandler::run: client handler thread finished\n\n\n");

    _isRunning = false;
}

bool ConnHandler::isRunning (void)
{
    return _isRunning;
}

ConnHandler::~ConnHandler (void)
{
    delete _pMocket;
    _pMocket = NULL;
}

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, Statistics *pStats, uint16 ui16Iterations, bool bReEstablish)
{
    int rc;
    static char buf [1024];
    static bool bBufInitialized;

    int64 i64StartTotalTime;
    int64 i64EndTime;


    if (!bBufInitialized) {
        srand (1234);
        for (int i = 0; i < sizeof (buf); i++) {
            buf[i] = (char) rand();
        }
        bBufInitialized = true;
    }

    Mocket *pm = new Mocket();
    pm->setIdentifier ("SuspendResumeTest-Client");
    pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    i64StartTotalTime = getTimeInMilliseconds();
    // Call connect() with bSupportConnectivity=true
    rc = pm->connect (pszRemoteHost, usRemotePort, true, 0);
    i64EndTime = getTimeInMilliseconds();
    if (0 != rc) {
        fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                 pszRemoteHost, usRemotePort, rc);
        printf ("doClientTask: Unable to connect\n");
        delete pm;
        return -1;
    }
    printf ("TIME TO CONNECT ** %llu ms **\n", (i64EndTime - i64StartTotalTime));

    int iDataSize = DATA_SIZE;
    int iBytesSent = 0;
    int iDataSizeInNBO = EndianHelper::htonl ((uint32)iDataSize);
    char chReply = 0;

    // reliable sequenced packets
    MessageSender sender = pm->getSender (true, true);
    printf ("doClientTask: Sending info on next block of data to send...\n");
    sender.send (&iDataSizeInNBO, sizeof (iDataSizeInNBO));
    printf ("doClientTask: Receiving acknowledgement...\n");
    pm->receive (&chReply, 1);
    if (chReply != '.') {
        fprintf (stderr, "doClientTask: failed to receive . from remote host\n");
        return -2;
    }

    i64StartTotalTime = getTimeInMilliseconds();
    // Send data
    for (int i = 0; i < (int) ui16Iterations; i++) {
        printf ("Iteration number %i Sending data...\n",i);
        while (iBytesSent < iDataSize) {
            sender.send (buf, sizeof (buf));
            iBytesSent += sizeof (buf);
            //printf ("doClientTask: written so far :: %d\n", iBytesSent);
        }

        i64EndTime = getTimeInMilliseconds();
        int iTime = (int) (getTimeInMilliseconds() - i64StartTotalTime);
        iBytesSent = 0;

        pStats->update ((double) (i64EndTime - i64StartTotalTime));
        sleepForMilliseconds(500);
    }

    sleepForMilliseconds(1000);

    if (bReEstablish) {
        printf ("doClientTask: Done sending. Now reEstablish the connection\n");
        int rcc;
        i64StartTotalTime = getTimeInMilliseconds();
        rcc = pm->reEstablishConn (2000);
        i64EndTime = getTimeInMilliseconds();
        printf ("Mocket reEstablishConn finished with status %d\n", rcc);
        printf ("TIME TO REESTABLISH ** %llu ms **\n", (i64EndTime - i64StartTotalTime));
        if (rcc != 0) {
            pm->close();
            delete pm;
            return -1;
        }
    }

    // Send data
    printf ("doClientTask: Start second sending cycle\n");
    for (int i = 0; i < (int) ui16Iterations; i++) {
        printf ("Iteration number %i Sending data...\n",i);
        while (iBytesSent < iDataSize) {
            sender.send (buf, sizeof (buf));
            iBytesSent += sizeof (buf);
            //printf ("doClientTask: written so far :: %d\n", iBytesSent);
        }

        iBytesSent = 0;

//        pStats->update ((double) (i64EndTime - ui64StartReEstablish));
        sleepForMilliseconds(1000);
    }

    printf ("doClientTask: Done sending\n\n");

    int iTotalTime = (int) (getTimeInMilliseconds() - i64StartTotalTime);
    //printf ("doClientTask: Total time to achive the estimated RTT value %i ms\n", iTotalTime);
    pm->close();
    delete pm;
    return 0;
}

int main (int argc, char *argv[])
{
    #if defined (WIN32) && defined (_DEBUG)
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
    #endif
    bool bServer = false;
    bool bClient = false;
    uint16 ui16Port = 1234;
    uint16 ui16Iterations = 1;
    String remoteHost;
    bool bReEstablish = false;

    int i = 1;
    while (i < argc) {
        if (0 == stricmp (argv[i], "-client")) {
            bClient = true;
        }
        else if (0 == stricmp (argv[i], "-server")) {
            bServer = true;
        }
        else if (0 == stricmp (argv[i], "-port")) {
            i++;
            if (i < argc) {
                ui16Port = (uint16) atoi (argv[i]);
            }
        }
        else if (0 == stricmp (argv[i], "-iterations")) {
            i++;
            if (i < argc) {
                ui16Iterations = (uint16) atoi (argv[i]);
            }
        }
        else if (0 == stricmp (argv[i], "-remotehost")) {
            i++;
            if (i < argc) {
                remoteHost = argv[i];
            }
        }
        else if (0 == stricmp (argv[i], "-reEstablish")) {
            bReEstablish = true;
        }

        i++;
    }
    bool bParamError = false;
    if ((bClient) && (bServer)) {
        bParamError = true;  // Cannot be both client and server
    }
    if ((!bClient) && (!bServer)) {
        bParamError = true;  // Must be either a client or a server
    }
    if ((bClient) && (remoteHost.length() <= 0)) {
        bParamError = true;  // Must specify remote host if this is the client
    }
    if (bParamError) {
        fprintf (stderr, "usage:\n %s {-server [-reEstablish] [-port <port>] }\n %s {-client -remotehost <remotehost> [-reEstablish] [-port <port>] [-iterations <iterations>]}\n", argv[0], argv[0]);
        return -1;
    }

    if (bServer) {
        // On a server - start up both MessageMocket

        FILE *fileLog = fopen ("stats-server-MsgMockets-cpp.txt", "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file stats-server-MsgMockets-cpp.txt\n");
            return -2;
        }
        fprintf (fileLog, "Delta Time, Time, SentPacketsCount, SentByteCount, ReceivedPacketsCount, ReceivedBytesCount, Retransmits, DuplicateDiscarded, NoRoomDiscarded\n");
        fclose (fileLog);

        printf ("\nCreating a ServerMocket on port %d\n", (int) ui16Port);
        ServerMocketThread *pSMT = new ServerMocketThread (ui16Port, bReEstablish);
        pSMT->start();
		sleepForMilliseconds(1000);
        while (pSMT->isRunning()) {
            sleepForMilliseconds(1000);
        }
        delete pSMT;
    }
    else if (bClient) {

        FILE *fileLog = fopen ("stats-client-MsgMockets-cpp.txt", "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file stats-client-MsgMockets-cpp.txt\n");
            return -3;
        }
        fprintf (fileLog, "Delta Time, Time, SentPacketsCount, SentByteCount, ReceivedPacketsCount, ReceivedBytesCount, Retransmits, DuplicateDiscarded, NoRoomDiscarded\n");
        fclose (fileLog);

        printf ("\nClient: number of iterations: %d\n", (int) ui16Iterations);
        printf ("Client: will connect to host %s on port %d\n", (const char*) remoteHost, (int) ui16Port);

        Statistics mocketStats;
        int rc;

        printf ("Starting Mocket Test...\n\n");
        if (0 != (rc = doClientTask (remoteHost, ui16Port, &mocketStats, ui16Iterations, bReEstablish))) {
            fprintf (stderr, "main: doClientTask failed for Mockets with rc = %d\n", rc);
            return -5;
        }
    }

    #if defined (WIN32) && defined (_DEBUG)
        _CrtDumpMemoryLeaks();
    #endif

    return 0;
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 10000) {
        printf ("closing connection after %lu ms\n", ulMilliSec);
        // The return value equal to true means that we are requesting to close the connection
        // Receiver takes care of closing it
        return true;
    }
    else {
        return false;
    }
}

bool suspendReceivedCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    printf ("Warning: the mocket has been suspended from: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 10000) {
        printf ("closing connection after %lu ms\n", ulMilliSec);
        // The return value equal to true means that we are requesting to close the connection
        // Receiver takes care of closing it
        return true;
    }
    else {
        return false;
    }
}
