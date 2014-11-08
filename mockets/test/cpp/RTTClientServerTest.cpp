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
        if (pMocket) {
            printf ("ServerMocket: got a connection\n");
            ConnHandler *pHandler = new ConnHandler (pMocket);
            pHandler->start();
        }
    }
}

ConnHandler::ConnHandler (Mocket *pMocket)
{
    _pMocket = pMocket;
}

void ConnHandler::run (void)
{
    int i;
    char buf[1024];
    int iBytesRead = 0;
    int iBytesToRead = 0;
    int iTime = 0;
    char chReply = '.';
    bool bNewDataToRead = false;
    int iIterNumber = 0;

    printf ("ConnHandler::run: client handler thread started for an incoming mockets connection\n");
    _pMocket->setIdentifier ("RTTCLientServer-Server");
    _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    _pMocket->receive (&iBytesToRead, sizeof (iBytesToRead));
    printf ("ConnHandler::run: Received request to read packets of [%d] bytes from client...\n", iBytesToRead);
    bNewDataToRead = true;
    iBytesToRead = EndianHelper::ntohl ((uint32)iBytesToRead);
    MessageSender sender = _pMocket->getSender (true, true);
    printf ("ConnHandler::run: Sending acknoledgement...\n");
    sender.send (&chReply, sizeof (chReply));

    
    printf("Start time %llu\n", getTimeInMilliseconds());
    while (bNewDataToRead) {
        //printf("\nIteration number %i\n", iIterNumber++);

        int64 i64StartTime = getTimeInMilliseconds();
        printf ("ConnHandler::run: Reading data...\n");
        while (iBytesRead < iBytesToRead) {
            if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
                iBytesRead += i;
                //printf ("ConnHandler::run: Read so far :: [%d]\n", iBytesRead);
            }
            else {
                printf ("ConnHandler::run: receive returned %d; no more data to read from client\n", i);
                bNewDataToRead = false;
                break;
            }
        }
        if (bNewDataToRead) {
            iTime = (int) (getTimeInMilliseconds() - i64StartTime);
            if (iTime == 0) {
                iTime = 1;
            }

            //printf ("ConnHandler::run: Read a total of [%d] bytes\n", iBytesRead);
            iBytesRead = 0;
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
}

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, Statistics *pStats, uint16 ui16SecondsToRun)
{
    int rc;
    static char buf [1024];
    static bool bBufInitialized;

    if (!bBufInitialized) {
        srand (1234);
        for (int i = 0; i < sizeof (buf); i++) {
            buf[i] = (char) rand();
        }
        bBufInitialized = true;
    }

    Mocket *pm = new Mocket();
    pm->setIdentifier ("RTTCLientServerTest-Client");
    pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    if (0 != (rc = pm->connect (pszRemoteHost, usRemotePort))) {
        fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                 pszRemoteHost, usRemotePort, rc);
        printf ("doClientTask: Unable to connect\n");
        delete pm;
        return -1;
    }
    
    int iDataSize = DATA_SIZE;
    int iBytesSent = 0;
    int iDataSizeInNBO = EndianHelper::htonl ((uint32)iDataSize);
    char chReply = 0;

    // reliable sequenced packets
    MessageSender sender = pm->getSender (true, true);
    printf("doClientTask: Sending info on next block of data to send...\n");
    sender.send (&iDataSizeInNBO, sizeof (iDataSizeInNBO));
    printf("doClientTask: Receiving acknowledgement...\n");
    pm->receive (&chReply, 1);
    if (chReply != '.') {
        fprintf (stderr, "doClientTask: failed to receive . from remote host\n");
        return -2;
    }
    
    int64 i64StartTotalTime = getTimeInMilliseconds();
    
    while (getTimeInMilliseconds() < (i64StartTotalTime + ui16SecondsToRun*1000)) {
        int64 i64StartTime = getTimeInMilliseconds();
        printf("doClientTask: Sending data...\n");
        while (iBytesSent < iDataSize) {
            sender.send (buf, sizeof (buf));
            iBytesSent += sizeof (buf);
            printf ("doClientTask: written so far :: %d\n", iBytesSent);
        }
       
        int64 i64EndTime = getTimeInMilliseconds();
        int iTime = (int) (getTimeInMilliseconds() - i64StartTime);
        iBytesSent = 0;

        pStats->update ((double) (i64EndTime - i64StartTime));

        // Save results to a file
        FILE *file = fopen ("stats-client-MsgMockets-cpp.txt", "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file stats-MsgMockets-cpp.txt\n");
            return -3;
        }
        fprintf (file, "%lu, %d, %d, %d, %d, %d, %d, %d, %d\n", (unsigned long) (getTimeInMilliseconds()/1000), iTime, 
                 pm->getStatistics()->getSentPacketCount(),
                 pm->getStatistics()->getSentByteCount(),
                 pm->getStatistics()->getReceivedPacketCount(),
                 pm->getStatistics()->getReceivedByteCount(),
                 pm->getStatistics()->getRetransmitCount(),
                 pm->getStatistics()->getDuplicatedDiscardedPacketCount(),
                 pm->getStatistics()->getNoRoomDiscardedPacketCount());
        fclose (file);
    }
    int iTotalTime = (int) (getTimeInMilliseconds() - i64StartTotalTime);
    printf ("doClientTask: Total time to achive the estimated RTT value %i ms\n", iTotalTime);
    printf ("Mocket closed with status %d\n",pm->close());
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
    bool bNoLogging = false;
    uint16 ui16Port = 1234;
    uint16 ui16SecondsToRun = 0;
    String remoteHost;

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
        else if (0 == stricmp(argv[i], "-seconds")) {
            i++;
            if (i < argc) {
                ui16SecondsToRun = (uint16) atoi (argv[i]);
            }
        }
        else if (0 == stricmp (argv[i], "-remotehost")) {
            i++;
            if (i < argc) {
                remoteHost = argv[i];
            }
        }
        else if ((0 == stricmp (argv[i], "-nolog")) || (0 == stricmp (argv[i], "-nologging"))) {
            bNoLogging = true;
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
        fprintf (stderr, "usage: %s {-server | -client -remotehost <remotehost>} [-port <port>] [-seconds <seconds to run>] [-nolog|-nologging]\n", argv[0]);
        return -1;
    }

    if (bServer) {
        // On a server - start up both MessageMocket
        if (!bNoLogging) {
            pLogger = new Logger();
            pLogger->initLogFile ("RTTCLientServerTest-server.log");
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();
        }

        FILE *fileLog = fopen ("stats-server-MsgMockets-cpp.txt", "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file stats-server-MsgMockets-cpp.txt\n");
            return -2;
        }
        fprintf (fileLog, "Delta Time, Time, SentPacketsCount, SentByteCount, ReceivedPacketsCount, ReceivedBytesCount, Retransmits, DuplicateDiscarded, NoRoomDiscarded\n");
        fclose (fileLog);

        printf ("\nCreating a ServerMocket on port %d\n", (int) ui16Port);
        ServerMocketThread *pSMT = new ServerMocketThread (ui16Port);
        pSMT->run();

    }
    else if (bClient) {
        if (!bNoLogging) {
            pLogger = new Logger();
            pLogger->initLogFile ("RTTCLientServerTest-client.log");
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();
        }

        FILE *fileLog = fopen ("stats-client-MsgMockets-cpp.txt", "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file stats-client-MsgMockets-cpp.txt\n");
            return -3;
        }
        fprintf (fileLog, "Delta Time, Time, SentPacketsCount, SentByteCount, ReceivedPacketsCount, ReceivedBytesCount, Retransmits, DuplicateDiscarded, NoRoomDiscarded\n");
        fclose (fileLog);

        printf ("\nClient: seconds to run: %d\n", (int) ui16SecondsToRun);
        printf ("Client: will connect to host %s on port %d\n", (const char*) remoteHost, (int) ui16Port);

        Statistics mocketStats;
        int rc;

        printf ("Starting Mocket Test...\n\n");
        if (0 != (rc = doClientTask (remoteHost, ui16Port, &mocketStats, ui16SecondsToRun))) {
            fprintf (stderr, "main: doClientTask failed for Mockets with rc = %d\n", rc);
            return -5;
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
    if (ulMilliSec > 60000) {
        printf ("closing connection after %lu ms\n", ulMilliSec);
        return true;
    }
    else {
        return false;
    }
}
