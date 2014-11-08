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
#include "TCPSocket.h"
#include "BufferedReader.h"
#include "BufferedWriter.h"
#include "SocketReader.h"
#include "SocketWriter.h"
#include "ObjectDefroster.h"
#include "ObjectFreezer.h"

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

bool _bSender;
bool _bReceiver;
bool _bReceiver2;

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
    if (_bSender) {
        int i;
        //Variable to read
        char buf[1024];
        // Variable to send
        //char sendbuf[1024];
        char sendbuf[10] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'l'};
        int rc;

        printf ("ConnHandler::run: client handler thread started for an incoming mockets connection\n");
        _pMocket->setIdentifier ("FreezeDefrost-Server");
        _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
        _pMocket->registerSuspendReceivedWarningCallback (suspendReceivedCallback, NULL);
        _pMocket->setConnectionLingerTime (1000); // in this way if there are data in the queues we don't wait indefinitely
        // USE THIS WHEN TESTING FOR DATA IN RECEIVER OR PACKETPROCESSOR
        //_pMocket->debugStateCapture();

        // Packets are reliable sequenced
        MessageSender sender = _pMocket->getSender (true, true);
        printf ("sendbuf = [%s] sizeOfBuf = %d\n", sendbuf, (int) sizeof (sendbuf));
        //!!// Send a bunch of data
        for (int c=0; c<15; c++) {
            sender.send (sendbuf, sizeof (sendbuf));
            printf ("Send data\n");
        }

        //!!// Read some data
        for (int c=0; c<5; c++) {
            if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
                printf ("Server: Read data\n");
            }
            else {
                printf ("ConnHandler::run: receive returned %d; no more data to read from client\n", i);
                break;
            }
        }

        // SUSPEND
        uint64 ui64StartSuspend = getTimeInMilliseconds();
        int res;
        res = _pMocket->suspend(0, 5000);
        printf ("Mocket suspend finished with status %d\n", res);
        printf ("TIME FOR SUSPENSION **  %llu  **\n", (getTimeInMilliseconds() - ui64StartSuspend));

        sleepForMilliseconds (2000);

        // Open a socket to transfer the data
        String strNextNodeHost = "127.0.0.1";
        int iNextNodePort = 4444;
        TCPSocket *pSocketToNextNode;
        pSocketToNextNode = new TCPSocket();
        while (true) {
            printf ("trying to connect to %s:%d...\n", (const char*) strNextNodeHost, iNextNodePort);
            fflush (stdout);
            if (0 != (rc = pSocketToNextNode->connect (strNextNodeHost, iNextNodePort))) {
                printf ("failed - will try again in 2 seconds\n");
                sleepForMilliseconds (2000);
            }
            else {
                printf ("Connected\n");
                break;
            }
        }

        SocketWriter sw (pSocketToNextNode);
        BufferedWriter bw (&sw, 2048);

        // Freeze
        int result;
        uint64 ui64StartFreeze = getTimeInMilliseconds();
        result = _pMocket->getState (&bw);
        printf ("TIME FOR GETSTATE **  %llu  **\n", (getTimeInMilliseconds() - ui64StartFreeze));
        printf ("getState ended with status %d\n", result);
        if (0 != result) {
            delete _pMocket;
            return;
        }
    }

    else if (_bReceiver) {
        int i;
        //Variable to read into
        char buf[1024];

        // Create the Server Socket
        int rc;
        TCPSocket *pServerSocket;
        pServerSocket = new TCPSocket();
        int iLocalPortNum = 4444;
        if (0 != (rc = pServerSocket->setupToReceive (iLocalPortNum))) {
            return;
        }
        printf ("listening on port %d\n", iLocalPortNum);

        Socket *pSocketFromPreviousNode;
        // Wait for connection from previous node
        printf ("Waiting for connection from previous node...\n");
        fflush (stdout);
        pSocketFromPreviousNode = pServerSocket->accept();
        if (pSocketFromPreviousNode == NULL) {
            printf ("Failed\n");
            return;
        }
        printf ("Connected\n");
        pSocketFromPreviousNode->bufferingMode(0);

        SocketReader sr (pSocketFromPreviousNode);
        BufferedReader br (&sr);

        // Resume
        int result;
        uint64 ui64StartResumeAndRestoreState = getTimeInMilliseconds();
        result = _pMocket->resumeAndRestoreState (&br);
        printf ("TIME FOR RESUMEANDRESTORESTATE **  %llu  **\n", (getTimeInMilliseconds() - ui64StartResumeAndRestoreState));
        printf ("resumeFromSuspension ended with status %d\n", result);

        if (0 != result) {
            printf ("ERROR\n");
            delete _pMocket;
            return;
        }

        //!!// Read data
        for (int k=0; k<5; k++) {
            if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
                printf ("Server: Read data\n");
            }
            else {
                printf ("ConnHandler::run: receive returned %d; no more data to read from client\n", i);
                break;
            }
        }

        /*********************/
        // SUSPEND
        uint64 ui64StartSuspend = getTimeInMilliseconds();
        int res;
        res = _pMocket->suspend(0, 5000);
        printf ("Mocket suspend finished with status %d\n", res);
        printf ("TIME FOR SUSPENSION **  %llu  **\n", (getTimeInMilliseconds() - ui64StartSuspend));

        sleepForMilliseconds (2000);

        // Open a socket to transfer the data
        String strNextNodeHost = "127.0.0.1";
        int iNextNodePort = 4445;
        TCPSocket *pSocketToNextNode;
        pSocketToNextNode = new TCPSocket();
        while (true) {
            printf ("trying to connect to %s:%d...\n", (const char*) strNextNodeHost, iNextNodePort);
            fflush (stdout);
            if (0 != (rc = pSocketToNextNode->connect (strNextNodeHost, iNextNodePort))) {
                printf ("failed - will try again in 2 seconds\n");
                sleepForMilliseconds (2000);
            }
            else {
                printf ("Connected\n");
                break;
            }
        }

        SocketWriter sw (pSocketToNextNode);
        BufferedWriter bw (&sw, 2048);

        // Freeze
        uint64 ui64StartFreeze = getTimeInMilliseconds();
        result = _pMocket->getState (&bw);
        printf ("TIME FOR GETSTATE **  %llu  **\n", (getTimeInMilliseconds() - ui64StartFreeze));
        printf ("getState ended with status %d\n", result);
        if (0 != result) {
            delete _pMocket;
            return;
        }

    }

    else if (_bReceiver2) {
        int i;
        //Variable to read into
        char buf[1024];

        // Create the Server Socket
        int rc;
        TCPSocket *pServerSocket;
        pServerSocket = new TCPSocket();
        int iLocalPortNum = 4445;
        if (0 != (rc = pServerSocket->setupToReceive (iLocalPortNum))) {
            return;
        }
        printf ("listening on port %d\n", iLocalPortNum);

        Socket *pSocketFromPreviousNode;
        // Wait for connection from previous node
        printf ("Waiting for connection from previous node...\n");
        fflush (stdout);
        pSocketFromPreviousNode = pServerSocket->accept();
        if (pSocketFromPreviousNode == NULL) {
            printf ("Failed\n");
            return;
        }
        printf ("Connected\n");
        pSocketFromPreviousNode->bufferingMode(0);

        SocketReader sr (pSocketFromPreviousNode);
        BufferedReader br (&sr);

        // Resume
        int result;
        uint64 ui64StartResumeAndRestoreState = getTimeInMilliseconds();
        result = _pMocket->resumeAndRestoreState (&br);
        printf ("TIME FOR RESUMEANDRESTORESTATE **  %llu  **\n", (getTimeInMilliseconds() - ui64StartResumeAndRestoreState));
        printf ("resumeFromSuspension ended with status %d\n", result);

        if (0 != result) {
            printf ("ERROR\n");
            delete _pMocket;
            return;
        }

        //!!// Read data
        while (true) {
            if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
                printf ("Server: Read data\n");
            }
            else {
                printf ("ConnHandler::run: receive returned %d; no more data to read from client\n", i);
                break;
            }
        }
    delete _pMocket;
    }
    printf ("ConnHandler::run: client handler thread finished\n\n\n");
}

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, Statistics *pStats)
{
    int rc;
    // Variable to send
    char buf [10] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'l'};
    // Variable to read
    char readbuf[1024];

    printf ("buf = [%s] sizeOfBuf = %d\n", buf, (int) sizeof (buf));

    Mocket *pm = new Mocket();

    pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    pm->setIdentifier ("FreezeDefrost-Client");


    if (0 != (rc = pm->connect (pszRemoteHost, usRemotePort, true))) {
        fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                 pszRemoteHost, usRemotePort, rc);
        printf ("doClientTask: Unable to connect\n");
        delete pm;
        return -1;
    }
    else {
        printf ("Connected\n");
    }

    // reliable sequenced packets
    MessageSender sender = pm->getSender (true, true);
      
    //!!// Read a bunch of data
    int receive;
    for (int c=0; c<15; c++) {
       receive = pm->receive (readbuf, sizeof (readbuf));
       printf ("Read data: %d byte\n", receive);
    }

    //!!// Send data
    while (true) {
        printf ("doClientTask: Sending data...\n");
        sender.send (buf, sizeof (buf));
        sleepForMilliseconds (1000);
    }

    sleepForMilliseconds (2000);

    printf ("Client thread finished\n");
    
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
    String remoteHost;
    _bSender = false;
    _bReceiver = false;

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
        else if (0 == stricmp (argv[i], "-remotehost")) {
            i++;
            if (i < argc) {
                remoteHost = argv[i];
            }
        }
        else if (0 == stricmp(argv[i], "-sender")) {
            _bSender = true;
        }
        else if (0 == stricmp(argv[i], "-receiver")) {
            _bReceiver = true;
        }
        else if (0 == stricmp(argv[i], "-receiver2")) {
            _bReceiver2 = true;
        }

        i++;
    }
    bool bParamError = false;
    if ((bClient) && (bServer)) {
        printf ("ERROR: Cannot be both client and server\n");
        bParamError = true;  // Cannot be both client and server
    }
    if ((!bClient) && (!bServer)) {
        printf ("ERROR: Must be either a client or a server\n");
        bParamError = true;  // Must be either a client or a server
    }
    if ((bClient) && (remoteHost.length() <= 0)) {
        printf ("ERROR: Must specify remote host if this is the client\n");
        bParamError = true;  // Must specify remote host if this is the client
    }
    if (bServer) {
        if ((!_bSender) && (!_bReceiver) && (!_bReceiver2)) {
            printf ("ERROR: Must be either sender or receiver of the mocket migration\n");
            bParamError = true;  // Must be either sender or receiver of the mocket migration
        }
    }
    if (bParamError) {
        fprintf (stderr, "usage: %s {-server -sender | -receiver [-port <port>] | -client -remotehost <remotehost> [-port <port>]}\n", argv[0]);
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

        if (_bSender) {
            printf ("\nCreating a ServerMocket on port %d\n", (int) ui16Port);
            ServerMocketThread *pSMT = new ServerMocketThread (ui16Port);
            pSMT->run();
        }
        else if (_bReceiver) {
            printf ("\nReceiver Server Side skip creating a new server mocket just resume the connection\n");
            Mocket *pm = new Mocket();
            ConnHandler *pCH = new ConnHandler (pm);
            pCH->run();
        }
        else if (_bReceiver2) {
            printf ("\nReceiver Server Side skip creating a new server mocket just resume the connection\n");
            Mocket *pm = new Mocket();
            ConnHandler *pCH = new ConnHandler (pm);
            pCH->run();
        }

    }
    else if (bClient) {

        FILE *fileLog = fopen ("stats-client-MsgMockets-cpp.txt", "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file stats-client-MsgMockets-cpp.txt\n");
            return -3;
        }
        fprintf (fileLog, "Delta Time, Time, SentPacketsCount, SentByteCount, ReceivedPacketsCount, ReceivedBytesCount, Retransmits, DuplicateDiscarded, NoRoomDiscarded\n");
        fclose (fileLog);

        printf ("Client: will connect to host %s on port %d\n", (const char*) remoteHost, (int) ui16Port);

        Statistics mocketStats;
        int rc;

        printf ("Starting Mocket Test...\n\n");
        if (0 != (rc = doClientTask (remoteHost, ui16Port, &mocketStats))) {
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
    if (ulMilliSec > 60000) {
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
    if (ulMilliSec > 60000) {
        printf ("closing connection after %lu ms\n", ulMilliSec);
        // The return value equal to true means that we are requesting to close the connection
        // Receiver takes care of closing it
        return true;
    }
    else {
        return false;
    }
}


