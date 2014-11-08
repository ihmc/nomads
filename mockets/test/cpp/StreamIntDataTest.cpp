#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "InetAddr.h"
//#include "Logger.h"
#include "StreamMocket.h"
#include "NLFLib.h"
#include "StreamServerMocket.h"
#include "Socket.h"
#include "TCPSocket.h"
#include "Thread.h"

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

using namespace NOMADSUtil;        
        
bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

class Stats
{
    public:
        Stats (void);
        void update (double value);
        void reset (void);
        int getNumValues (void);
        double getAverage (void);
        double getStDev (void);
    private:
        double _sumValues;
        double _sumSqValues;
        int _totalNumValues;
};

Stats::Stats (void)
{
    _sumValues = 0.0;
    _sumSqValues = 0.0;
    _totalNumValues = 0;
}

void Stats::update (double value)
{
    _sumValues += value;
    _sumSqValues += (value*value);
    _totalNumValues++;
}

void Stats::reset (void)
{
    _sumValues = 0;
    _sumSqValues = 0;
    _totalNumValues = 0;
}

int Stats::getNumValues (void)
{
    return _totalNumValues;
}

double Stats::getAverage (void)
{
    return (_sumValues/_totalNumValues);
}

double Stats::getStDev (void)
{
    double avg = getAverage();
    double aux = (_totalNumValues * avg * avg)
                  - (2 * avg * _sumValues)
                  + _sumSqValues;
    aux = (double) aux / (_totalNumValues - 1);
    aux = sqrt (aux);
    return aux;
}

class StreamServerMocketThread : public Thread
{
    public:
        StreamServerMocketThread (int usServerPort);
        void run (void);

    private:
        StreamServerMocket *_pStreamServerMocket;
};

class ServerSocketThread : public Thread
{
    public:
        ServerSocketThread (int usServerPort);
        void run (void);

    private:
        TCPSocket *_pServerSocket;
};

class ConnHandler : public Thread
{
    public:
        ConnHandler (StreamMocket *pMocket);
        ConnHandler (Socket *pSocket);
        void run (void);

    private:
        StreamMocket *_pMocket;
        Socket *_pSocket;
        bool _useMockets;
};

StreamServerMocketThread::StreamServerMocketThread (int usServerPort)
{
    _pStreamServerMocket = new StreamServerMocket();
    _pStreamServerMocket->listen (usServerPort);
}

void StreamServerMocketThread::run (void)
{
    while (true) {
        StreamMocket *pMocket = _pStreamServerMocket->accept();
        if (pMocket) {
            printf ("StreamServerMocket: got a connection\n");
            ConnHandler *pHandler = new ConnHandler (pMocket);
            pHandler->start();
        }
    }
}

ServerSocketThread::ServerSocketThread (int usServerPort)
{
    _pServerSocket = new TCPSocket();
    _pServerSocket->setupToReceive (usServerPort);
}

void ServerSocketThread::run (void)
{
    while (true) {
        Socket *pSocket = _pServerSocket->accept();
        if (pSocket) {
            printf ("ServerSocket: got a connection\n");
            ConnHandler *pHandler = new ConnHandler (pSocket);
            pHandler->start();
        }
    }
}

ConnHandler::ConnHandler (StreamMocket *pMocket)
{
    _pMocket = pMocket;
    _pSocket = NULL;
    _useMockets = true;
}

ConnHandler::ConnHandler (Socket *pSocket)
{
    _pMocket = NULL;
    _pSocket = pSocket;
    _useMockets = false;
}

void ConnHandler::run (void)
{
    int i;
    char buf[1024];
    int iBytesRead = 0;
    int iBytesToRead = 0;
    int iTime = 0;
    char chReply = '.';

    printf ("ConnectionHandler: client handler thread started\n");
    int64 i64StartTime = getTimeInMilliseconds();
    if (_useMockets) {
        _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
        if (sizeof (iBytesToRead) != _pMocket->receive (&iBytesToRead, sizeof (iBytesToRead))) {
            printf ("ConnHandler::run: receive failed to read size of data; terminating\n");
            _pMocket->close();
            delete _pMocket;
            return;
        }
        //printf("will read %d bytes.\n", iBytesToRead);
        while (iBytesRead < iBytesToRead) {
            if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
                iBytesRead += i;
				//printf ("bytes read: [%d],    read so far :: [%d]\n", i, iBytesRead);
            }
            else {
                printf ("receive returned %d; closing connection\n", i);
                break;
            }
        }
        //printf ("Server before sending chReply\n");
        iTime = (int) (getTimeInMilliseconds() - i64StartTime);
        if (iTime == 0) {
            iTime = 1;
        }
        _pMocket->send (&chReply, sizeof (chReply));
        _pMocket->close();

        // Save results to a file
        FILE *file = fopen ("stats-server-StreamMockets-cpp.txt", "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file stats-mockets-cpp.txt\n");
            return;
        }
    	fprintf (file, "[%lu]\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", (unsigned long) (getTimeInMilliseconds()/1000), iTime, 
                 _pMocket->getStatistics()->getSentPacketCount(),
                 _pMocket->getStatistics()->getSentByteCount(),
                 _pMocket->getStatistics()->getReceivedPacketCount(),
                 _pMocket->getStatistics()->getReceivedByteCount(),
                 _pMocket->getStatistics()->getRetransmitCount(),
                 _pMocket->getStatistics()->getDuplicatedDiscardedPacketCount(),
                 _pMocket->getStatistics()->getNoRoomDiscardedPacketCount());

        fclose (file);

        delete _pMocket;
    }
    else {
        if (sizeof (iBytesToRead) != _pSocket->receive (&iBytesToRead, sizeof (iBytesToRead))) {
            printf ("ConnHandler::run: receive failed to read size of data; terminating\n");
            _pSocket->disconnect();
            delete _pSocket;
            return;
        }
        while (iBytesRead < iBytesToRead) {
            if ((i = _pSocket->receive (buf, sizeof (buf))) > 0) {
                iBytesRead += i;
            }
            else {
                printf ("receive returned %d; closing connection\n", i);
                break;
            }
        }
        iTime = (int) (getTimeInMilliseconds() - i64StartTime);
        if (iTime == 0) {
            iTime = 1;
        }        
        _pSocket->send (&chReply, sizeof (chReply));
        _pSocket->disconnect();
        
        // Save results to a file
        FILE *socfile = fopen ("statsSM-server-sockets-cpp.txt", "a");
        if (socfile == NULL) {
            fprintf (stderr, "failed to append to file stats-mockets-cpp.txt\n");
            return;
        }
    	fprintf (socfile, "[%lu]\t%d\t\n", (unsigned long) (getTimeInMilliseconds()/1000), iTime);

        fclose (socfile);

        delete _pSocket;
    }
    printf ("ConnectionHandler: client handler thread finished\n");
}

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, bool bUseMockets, Stats *pStats)
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

    if (bUseMockets) {
        StreamMocket mocket;
        if (0 != (rc = mocket.connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using mockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            return -1;
        }
        mocket.registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

        int iDataSize = 1024*1024;
        int iBytesSent = 0;
        int64 i64StartTime = getTimeInMilliseconds();
        mocket.send (&iDataSize, sizeof (iDataSize));
        while (iBytesSent < iDataSize) {
            mocket.send (buf, sizeof (buf));
            iBytesSent += sizeof (buf);
        }
        char chReply = 0;
        mocket.receive (&chReply, 1);
        if (chReply != '.') {
            fprintf (stderr, "doClientTask: failed to receive . from remote host\n");
            return -2;
        }
        int64 i64EndTime = getTimeInMilliseconds();
        int iTime = (int) (getTimeInMilliseconds() - i64StartTime);

        pStats->update ((double) (i64EndTime - i64StartTime));
        // Save results to a file
        FILE *file = fopen ("stats-client-streamMockets-cpp.txt", "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file stats-mockets-cpp.txt\n");
            return -3;
        }
        fprintf (file, "[%lu]\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", (unsigned long) (getTimeInMilliseconds()/1000), iTime, 
                 mocket.getStatistics()->getSentPacketCount(),
                 mocket.getStatistics()->getSentByteCount(),
                 mocket.getStatistics()->getReceivedPacketCount(),
                 mocket.getStatistics()->getReceivedByteCount(),
                 mocket.getStatistics()->getRetransmitCount(),
                 mocket.getStatistics()->getDuplicatedDiscardedPacketCount(),
                 mocket.getStatistics()->getNoRoomDiscardedPacketCount());
                 /*mocket.getStatistics()->getDiscardedPacketCounts()._iBelowWindow,
                 mocket.getStatistics()->getDiscardedPacketCounts()._iNoRoom,
                 mocket.getStatistics()->getDiscardedPacketCounts()._iOverlap,
                 mocket.getStatistics()->getTransmitterWaitCounts()._iPacketQueueFull,
                 mocket.getStatistics()->getTransmitterWaitCounts()._iRemoteWindowFull);*/

        fclose (file);

        mocket.close();
    }
    else {
        TCPSocket socket;
        if (0 != (rc = socket.connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using sockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            return -3;
        }

        int iDataSize = 1024*1024;
        int iBytesSent = 0;
        int64 i64StartTime = getTimeInMilliseconds();
        socket.send (&iDataSize, sizeof (iDataSize));
        while (iBytesSent < iDataSize) {
            socket.send (buf, sizeof (buf));
            iBytesSent += sizeof (buf);
        }
        char chReply = 0;
        socket.receive (&chReply, 1);
        if (chReply != '.') {
            fprintf (stderr, "doClientTask: failed to receive . from remote host\n");
            return -4;
        }
        int64 i64EndTime = getTimeInMilliseconds();
        int iTime = (int) (getTimeInMilliseconds() - i64StartTime);

        pStats->update ((double) (i64EndTime - i64StartTime));

        // Save results to a file
        FILE *socfile = fopen ("statsSM-client-sockets-cpp.txt", "a");
        if (socfile == NULL) {
            fprintf (stderr, "failed to append to file statsSM-mockets-cpp.txt\n");
            return -3;
        }
    	fprintf (socfile, "[%lu]\t%d\t\n", (unsigned long) (getTimeInMilliseconds()/1000), iTime);

        fclose (socfile);

        socket.disconnect();
    }
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

    /*pLogger = new Logger();
    pLogger->initLogFile ("AdHocTest.log");
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();*/

    if (((argc != 3) && (argc != 4)) && ((argc != 3) && (argc != 5))) {
        fprintf (stderr, "usage: %s <server|client> <port> [<remotehost> [<iterations>] ]\n", argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            // On a server - start up both mocket and socket
            unsigned short usPort = atoi (argv[2]);
            
            printf ("Creating a StreamServerMocket and ServerSocket on port %d\n", (int) usPort);
            StreamServerMocketThread *pSMT = new StreamServerMocketThread (usPort);
            pSMT->start();
            ServerSocketThread *pSST = new ServerSocketThread (usPort);
	    // Call run() instead of start() so that the main thread does not end
            pSST->run(); 
        }
        else if (0 == stricmp (argv[1], "client")) {
            unsigned short usRemotePort = atoi (argv[2]);
            unsigned short usIterations;
            if (argc == 5) {
                usIterations = atoi (argv[4]);

            }
            else {
                usIterations = 1000;
            }

            const char *pszRemoteHost = argv[3];
            printf ("-->>ClientIterations Number: %d\n", (int) usIterations);
            printf ("Client Creating a Mocket and Socket on port %d\n", (int) usRemotePort);

            Stats mocketStats, socketStats;
            for (int i = 0; i < usIterations; i++) {
                int rc;
                if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, false, &socketStats))) {
                    fprintf (stderr, "main: doClientTask failed for sockets with rc = %d\n", rc);
                    return -2;
                }
                if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, true, &mocketStats))) {
                    fprintf (stderr, "main: doClientTask failed for mockets with rc = %d\n", rc);
                    return -3;
                }
                printf ("-----------------------------------------\n");
                printf ("TotalAttempts: %d\n", i+1);
                printf ("StreamMocket Stats:: Average:       %10.4f\n", mocketStats.getAverage());
                printf ("StreamMocket Stats:: St Deviation:  %10.4f\n", mocketStats.getStDev());
                printf ("Socket Stats:: Average:       %10.4f\n", socketStats.getAverage());
                printf ("Socket Stats:: St Deviation:  %10.4f\n", socketStats.getStDev());
                printf ("-----------------------------------------\n");
                printf ("Sleeping for 10 seconds...\n");
                sleepForMilliseconds (10000);
            }
        }
    }

	#if defined (WIN32) && defined (_DEBUG)
        _CrtDumpMemoryLeaks();
    #endif

    return 0;
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    //printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 10000) {
        return true;
    }
    else {
        return false;
    }
}
