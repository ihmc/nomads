#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <vector>

#include "Mutex.h"
#include "AtomicVar.h"
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

#define MAX_MESSAGE_SIZE 2048
#define DEFAULT_TOTAL_SIZE_IN_KB 1024
#define DEFAULT_MESSAGE_SIZE_IN_BYTES 1024
#define SECS_TO_SLEEP 2

using namespace std;
using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
double doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, bool bUseMockets, bool bEnableKeyExchange, uint32 ui32SizeInKB, uint16 ui16MessageSizeInBytes, Statistics *pStats);

class Constants
{
public:
    static const String socketsServerStatsFile;
    static const String socketsClientStatsFile;
    static const String mocketsServerStatsFile;
    static const String mocketsClientStatsFile;
    static AtomicVar<double> defaultStatsVar;
};

const String Constants::socketsServerStatsFile("stats-server-sockets-cpp.txt");
const String Constants::socketsClientStatsFile("stats-Client-sockets-cpp.txt");
const String Constants::mocketsServerStatsFile("stats-server-MsgMockets-cpp.txt");
const String Constants::mocketsClientStatsFile("stats-Client-MsgMockets-cpp.txt");
AtomicVar<double> Constants::defaultStatsVar (0.0);


void logErrorToLogFile (const char * const pszLogFileName, int errorCode) {
    // Print error line to file
    FILE *file = fopen (pszLogFileName, "a");
    if (file == NULL) {
        fprintf (stderr, "failed to append to file %s\n", pszLogFileName);
    }
    fprintf (file, "An error occurred during data transfer. Reported error code is %d\n", errorCode);

    fclose (file);
}

class ServerMocketThread : public Thread
{
    public:
        ServerMocketThread (int usServerPort);
        void run (void);

    private:
        ServerMocket *_pServerMocket;
};

class ServerSocketThread : public Thread
{
    public:
        ServerSocketThread (int usServerPort);
        void run (void);

    private:
        TCPSocket *_pServerSocket;
};

class ClientTaskThread : public Thread
{
public:
    ClientTaskThread (uint16 ui16ThreadID, const char * const pszRemoteHost, unsigned short usRemotePort, bool bUseMockets, bool bEnableKeyExchange,
                      uint32 ui32SizeInKB, uint16 ui16MessageSizeInBytes, Statistics * const pStats, AtomicVar<double> &avThroughputStats);
    ClientTaskThread (const ClientTaskThread &rhs);

    ClientTaskThread & operator= (const ClientTaskThread &rhs);

    void run(void);

private:
    uint16 _ui16ThreadID;
    const char *_pszRemoteHost;
    unsigned short _usRemotePort;
    bool _bUseMockets;
    bool _bEnableKeyExchange;
    uint32 _ui32SizeInKB;
    uint16 _ui16MessageSizeInBytes;
    Statistics *_pStats;

    AtomicVar<double> &_avThroughputStats;
};

class ConnHandler : public Thread
{
    public:
        ConnHandler (Mocket *pMocket);
        ConnHandler (Socket *pSocket);
        void run (void);

    private:
        Mocket *_pMocket;
        Socket *_pSocket;
        bool _useMockets;
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

ConnHandler::ConnHandler (Mocket *pMocket)
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
    static char buf[MAX_MESSAGE_SIZE];
    static const char chReply = '.';
    int iBytesRead = 0, iBytesToRead = 0;
    unsigned int iTime = 0;
    int64 i64StartTime;

    if (_useMockets) {
        printf ("ConnHandler::run: client handler thread started for an incoming mockets connection\n");
        _pMocket->setIdentifier ("IntDataTest-Server");
        _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
        if (sizeof (iBytesToRead) != _pMocket->receive (&iBytesToRead, sizeof (iBytesToRead))) {
            printf ("ConnHandler::run: receive failed to read size of data; terminating\n");
            _pMocket->close();
            delete _pMocket;
            return;
        }
        iBytesToRead = EndianHelper::ntohl ((uint32) iBytesToRead);
        printf ("ConnHandler::run: will read [%d] bytes from client; remote port = %d\n", iBytesToRead, _pMocket->getRemotePort());
        MessageSender sender = _pMocket->getSender (true, true);
        sender.send (&chReply, sizeof (chReply));

        i64StartTime = getTimeInMilliseconds();
        while (iBytesRead < iBytesToRead) {
            if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
                iBytesRead += i;
            }
            else {
                printf ("ConnHandler::run: receive returned %d; closing connection\n", i);
                break;
            }
        }
        iTime = (unsigned int) (getTimeInMilliseconds() - i64StartTime);
        sender.send (&chReply, sizeof (chReply));
        
        printf ("ConnHandler::run: read a total of [%d] bytes; sending ACK\n", iBytesRead);
        if (iTime == 0) {
            iTime = 1;
        }

        // Save results to a file
        FILE *file = fopen(Constants::mocketsServerStatsFile.c_str(), "a");
        if (file == NULL) {
            fprintf(stderr, "failed to append to file %s\n", Constants::mocketsServerStatsFile.c_str());
            return;
        }

        fprintf (file, "%lu, %d, %d, %d, %d, %d, %d, %d, %d\n", (unsigned long) (getTimeInMilliseconds() / 1000), iTime, 
                 _pMocket->getStatistics()->getSentPacketCount(),
                 _pMocket->getStatistics()->getSentByteCount(),
                 _pMocket->getStatistics()->getReceivedPacketCount(),
                 _pMocket->getStatistics()->getReceivedByteCount(),
                 _pMocket->getStatistics()->getRetransmitCount(),
                 _pMocket->getStatistics()->getDuplicatedDiscardedPacketCount(),
                 _pMocket->getStatistics()->getNoRoomDiscardedPacketCount());
                 /*_pMocket->getStatistics()->getDiscardedPacketCounts()._iBelowWindow,
                 _pMocket->getStatistics()->getDiscardedPacketCounts()._iNoRoom,
                 _pMocket->getStatistics()->getDiscardedPacketCounts()._iOverlap,
                 _pMocket->getStatistics()->getTransmitterWaitCounts()._iPacketQueueFull,
                 _pMocket->getStatistics()->getTransmitterWaitCounts()._iRemoteWindowFull);*/

        fclose (file);
		_pMocket->close();
        delete _pMocket;
    }
    else {
        printf ("ConnHandler::run: client handler thread started for an incoming sockets connection\n");
        if (sizeof (iBytesToRead) != _pSocket->receive (&iBytesToRead, sizeof (iBytesToRead))) {
            printf ("ConnHandler::run: receive failed to read size of data; terminating\n");
            _pSocket->disconnect();
            delete _pSocket;
            return;
        }
        iBytesToRead = EndianHelper::ntohl ((uint32)iBytesToRead);
        printf ("ConnHandler::run: will read [%d] bytes from client\n", iBytesToRead);
        _pSocket->send (&chReply, sizeof (chReply));

        i64StartTime = getTimeInMilliseconds();
        while (iBytesRead < iBytesToRead) {
            if ((i = _pSocket->receive (buf, sizeof (buf))) > 0) {
                iBytesRead += i;
            }
            else {
                printf ("ConnHandler::run: receive returned %d; closing connection\n", i);
                break;
            }
        }
        iTime = (int) (getTimeInMilliseconds() - i64StartTime);

        _pSocket->send (&chReply, sizeof (chReply));
        _pSocket->disconnect();
        delete _pSocket;

        printf ("ConnHandler::run: read a total of [%d] bytes; sending ACK\n", iBytesRead);
        if (iTime == 0) {
            iTime = 1;
        }

        // Save results to a file
        FILE *socfile = fopen(Constants::socketsServerStatsFile.c_str(), "a");
        if (socfile == NULL) {
            fprintf(stderr, "failed to append to file %s\n", Constants::socketsServerStatsFile.c_str());
            return;
        }
        fprintf (socfile, "%lu, %d\n", (unsigned long) (getTimeInMilliseconds()/1000), iTime);
        fclose (socfile);
    }

    printf ("ConnHandler::run: client handler thread finished\n");
}

ClientTaskThread::ClientTaskThread (uint16 ui16ThreadID, const char * const pszRemoteHost, unsigned short usRemotePort, bool bUseMockets, bool bEnableKeyExchange,
                                    uint32 ui32SizeInKB, uint16 ui16MessageSizeInBytes, Statistics * const pStats, AtomicVar<double> &avThroughputStats) :
    _ui16ThreadID(ui16ThreadID), _pszRemoteHost(pszRemoteHost), _usRemotePort(usRemotePort), _bUseMockets(bUseMockets), _bEnableKeyExchange(bEnableKeyExchange),
    _ui32SizeInKB(ui32SizeInKB), _ui16MessageSizeInBytes(ui16MessageSizeInBytes), _pStats(pStats), _avThroughputStats(avThroughputStats) {}

ClientTaskThread::ClientTaskThread (const ClientTaskThread &rhs) :
    _ui16ThreadID(rhs._ui16ThreadID), _pszRemoteHost(rhs._pszRemoteHost), _usRemotePort(rhs._usRemotePort), _bUseMockets(rhs._bUseMockets),
    _bEnableKeyExchange(rhs._bEnableKeyExchange), _ui32SizeInKB(rhs._ui32SizeInKB), _ui16MessageSizeInBytes(rhs._ui16MessageSizeInBytes),
    _pStats(rhs._pStats), _avThroughputStats(rhs._avThroughputStats) {}

ClientTaskThread & ClientTaskThread::operator= (const ClientTaskThread &rhs)
{
    _ui16ThreadID = rhs._ui16ThreadID;
    _pszRemoteHost = rhs._pszRemoteHost;
    _usRemotePort = rhs._usRemotePort;
    _bUseMockets = rhs._bUseMockets;
    _bEnableKeyExchange = rhs._bEnableKeyExchange;
    _ui32SizeInKB = rhs._ui32SizeInKB;
    _ui16MessageSizeInBytes = rhs._ui16MessageSizeInBytes;
    Statistics *_pStats = rhs._pStats;
    _avThroughputStats = rhs._avThroughputStats;

    return (*this);
}

void ClientTaskThread::run (void)
{
    double tp = doClientTask(_pszRemoteHost, _usRemotePort, _bUseMockets, _bEnableKeyExchange, _ui32SizeInKB, _ui16MessageSizeInBytes, _pStats);
    if (tp < 0) {
        fprintf(stderr, "Thread# %hu: client task using %s has failed\n", _ui16ThreadID, _bUseMockets ? "Mockets" : "Sockets");
        return;
    }

    _avThroughputStats += tp;
}

double doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, bool bUseMockets, bool bEnableKeyExchange, uint32 ui32SizeInKB, uint16 ui16MessageSizeInBytes, Statistics *pStats)
{
    int rc;
    double dThroughput;
    int64 i64StartTime, i64EndTime;
    unsigned int iTime;
    unsigned char *buf = new unsigned char[ui16MessageSizeInBytes];
    srand ((unsigned int) getTimeInMilliseconds());

    // Following two locks are used to avoid race conditions on Statistics *pStats.
    static Mutex socketsStatsMutex;
    static Mutex mocketsStatsMutex;

    // Fill the buffer with random data
    for (int i = 0; i < ui16MessageSizeInBytes; i++) {
        buf[i] = (char) rand();
    }

    if (bUseMockets) {
        Mocket pm;
        pm.setIdentifier ("IntDataTest-Client");
        pm.registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

        if (0 != (rc = pm.connect (pszRemoteHost, usRemotePort, bEnableKeyExchange))) {
            fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            printf ("doClientTask: Unable to connect\n");
            delete[] buf;
            return -1;
        }
        
        uint32 ui32DataSize = ui32SizeInKB * 1024;
        int iBytesSent = 0, iBytesToSend = 0;
        int iDataSizeInNBO = EndianHelper::htonl (ui32DataSize);
        char chReply = 0;
        
        MessageSender sender = pm.getSender (true, true);
        sender.send (&iDataSizeInNBO, sizeof (iDataSizeInNBO));
        
        pm.receive (&chReply, 1);
        if (chReply != '.') {
            fprintf (stderr, "doClientTask: failed to receive '.' from remote host\n");
            delete[] buf;
            return -2;
        }
        
        i64StartTime = getTimeInMilliseconds();
        while (iBytesSent < ui32DataSize) {
            #if defined (UNIX)
            iBytesToSend = std::min ((int) ui16MessageSizeInBytes, (int)(ui32DataSize - iBytesSent));
            #elif defined (WIN32)
            iBytesToSend = min ((int) ui16MessageSizeInBytes, (ui32DataSize - iBytesSent));
            #endif
            //sender.send (buf, iBytesToSend);
            pm.send (true, true, buf, iBytesToSend, 0, 5, 0, 0);
            iBytesSent += iBytesToSend;
        }
        rc = pm.receive (&chReply, 1);
        i64EndTime = getTimeInMilliseconds();
        pm.close();

        if (chReply != '.') {
            mocketsStatsMutex.lock();
            logErrorToLogFile (Constants::mocketsClientStatsFile.c_str(), rc);
            mocketsStatsMutex.unlock();
            fprintf (stderr, "doClientTask: failed to receive '.' from remote host\n");
            delete[] buf;
            return -3;
        }

        iTime = (unsigned int) (i64EndTime - i64StartTime);
        if (iTime == 0) {
            iTime = 1;
        }

        mocketsStatsMutex.lock();
        printf ("DataSize = %u bytes = %u Kbytes\n", ui32DataSize, ui32SizeInKB);
        printf ("Time = %dms\n", iTime);
        dThroughput = ((double) ui32DataSize) / iTime;
        printf ("Throughput (bytes/ms) = %.2f (%d/%d)\n", dThroughput, ui32DataSize, iTime);
        dThroughput = (dThroughput * 1000) / 1024;
        printf ("Throughput (Kbytes/s) = %.2f (%d/%.2f)\n", dThroughput, ui32SizeInKB, ((double) iTime) / 1000);

        pStats->update ((double) (i64EndTime - i64StartTime));

        // Save results to a file
        FILE *file = fopen (Constants::mocketsClientStatsFile.c_str(), "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file %s\n", Constants::mocketsClientStatsFile.c_str());
            delete[] buf;
            return -4;
        }
        fprintf (file, "%lu, %d, %d, %d, %d, %d, %d, %d, %d, %.2f\n", (unsigned long) (getTimeInMilliseconds() / 1000), iTime,
                 pm.getStatistics()->getSentPacketCount(),
                 pm.getStatistics()->getSentByteCount(),
                 pm.getStatistics()->getReceivedPacketCount(),
                 pm.getStatistics()->getReceivedByteCount(),
                 pm.getStatistics()->getRetransmitCount(),
                 pm.getStatistics()->getDuplicatedDiscardedPacketCount(),
                 pm.getStatistics()->getNoRoomDiscardedPacketCount(),
                 dThroughput);
                 /*pm.getStatistics()->getDiscardedPacketCounts()._iBelowWindow,
                 pm.getStatistics()->getDiscardedPacketCounts()._iNoRoom,
                 pm.getStatistics()->getDiscardedPacketCounts()._iOverlap,
                 pm.getStatistics()->getTransmitterWaitCounts()._iPacketQueueFull,
                 pm.getStatistics()->getTransmitterWaitCounts()._iRemoteWindowFull);*/
        fclose (file);

        mocketsStatsMutex.unlock();
    }
    else {
        TCPSocket socket;
		//socket.blockingMode(1);
		//socket.bufferingMode(0);
        if (0 != (rc = socket.connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using sockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            delete[] buf;
            return -5;
        }

        uint32 ui32DataSize = ui32SizeInKB * 1024;
        int iBytesSent = 0, iBytesToSend = 0;
        int iDataSizeInNBO = EndianHelper::htonl (ui32DataSize);
        char chReply = 0;
        
        socket.send (&iDataSizeInNBO, sizeof (iDataSizeInNBO));
        socket.receive (&chReply, 1);
        if (chReply != '.') {
            fprintf (stderr, "doClientTask: failed to receive '.' from remote host\n");
            delete[] buf;
            return -6;
        }
        
        i64StartTime = getTimeInMilliseconds();
        while (iBytesSent < ui32DataSize) {
            #if defined (UNIX)
            iBytesToSend = std::min ((int) ui16MessageSizeInBytes, (int)(ui32DataSize - iBytesSent));
            #elif defined (WIN32)
            iBytesToSend = min ((int) ui16MessageSizeInBytes, (ui32DataSize - iBytesSent));
            #endif
            socket.send (buf, iBytesToSend);
            iBytesSent += iBytesToSend;
        }
        socket.receive (&chReply, 1);
        i64EndTime = getTimeInMilliseconds();
        socket.disconnect();

        if (chReply != '.') {
            fprintf (stderr, "doClientTask: failed to receive '.' from remote host\n");
            delete[] buf;
            return -7;
        }

        iTime = (unsigned int) (i64EndTime - i64StartTime);
        if (iTime == 0) {
			iTime = 1;
		}

        socketsStatsMutex.lock();
        printf ("DataSize = %d bytes = %d Kbytes\n", ui32DataSize, ui32SizeInKB);
        printf ("Time = %dms\n", iTime);
        dThroughput = ((double) ui32DataSize) / iTime;
        printf ("Throughput (bytes/ms) = %.2f (%d/%d)\n", dThroughput, ui32DataSize, iTime);
        dThroughput = (dThroughput * 1000) / 1024;
        printf ("Throughput (Kbytes/s) = %.2f (%d/%.2f)\n", dThroughput, ui32SizeInKB, ((double) iTime) / 1000);

        pStats->update ((double) (i64EndTime - i64StartTime));

        // Save results to a file
        FILE *socfile = fopen (Constants::socketsClientStatsFile.c_str(), "a");
        if (socfile == NULL) {
            fprintf (stderr, "failed to append to file %s\n", Constants::socketsClientStatsFile.c_str());
            delete[] buf;
            return -8;
        }
        fprintf (socfile, "%lu, %d, %.2f\n", (unsigned long) (getTimeInMilliseconds() / 1000), iTime, dThroughput);
        fclose (socfile);

        socketsStatsMutex.unlock();
    }

    delete[] buf;
    return dThroughput;
}

int main (int argc, char *argv[])
{
    #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (146);
    #endif
    bool bServer = false;
    bool bClient = false;
    bool bMockets = false;
    bool bSockets = false;
    bool bNoLogging = false;
    uint16 ui16Port = 1234;
    uint16 ui16Iterations = 1;
    uint16 ui16ConcurrentFlows = 1;
    bool bEnableKeyExchange = false;
    uint32 ui32SizeInKB = DEFAULT_TOTAL_SIZE_IN_KB;
    uint16 ui16MessageSizeInBytes = DEFAULT_MESSAGE_SIZE_IN_BYTES;
    String remoteHost;
    

    int i = 1;
    while (i < argc) {
        if (0 == stricmp (argv[i], "-client")) {
            bClient = true;
        }
        else if (0 == stricmp (argv[i], "-server")) {
            bServer = true;
        }
        else if (0 == stricmp (argv[i], "-mockets")) {
            bMockets = true;
        }
        else if (0 == stricmp (argv[i], "-sockets")) {
            bSockets = true;
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
        else if (0 == stricmp(argv[i], "-concurrentFlows")) {
            i++;
            if (i < argc) {
                ui16ConcurrentFlows = (uint16) atoi(argv[i]);
            }
        }
        else if (0 == stricmp (argv[i], "-size")) {
            i++;
            if (i < argc) {
                ui32SizeInKB = (uint32) atoi (argv[i]);
            }
        }
        else if ((0 == stricmp (argv[i], "-msgsize")) || (0 == stricmp (argv[i], "-messagesize"))) {
            i++;
            if (i < argc) {
                ui16MessageSizeInBytes = (uint16) atoi (argv[i]);
                if (ui16MessageSizeInBytes > MAX_MESSAGE_SIZE) {
                    ui16MessageSizeInBytes = MAX_MESSAGE_SIZE;
                }
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
        else if (0 == stricmp (argv[i], "-enableMocketKeyExchange")) {
            bEnableKeyExchange = true;
        }
        i++;
    }

    bool bParamError = false;
    if ((bClient) && (bServer)) {
        bParamError = true;  // Cannot be both client and server
    }
    else if ((!bClient) && (!bServer)) {
        bParamError = true;  // Must be either a client or a server
    }
    else if ((bClient) && (remoteHost.length() <= 0)) {
        bParamError = true;  // Must specify remote host if this is the client
    }
    if (bParamError) {
        fprintf (stderr, "usage: %s {-server | -client -remotehost <remotehost>} [-mockets] [-sockets] [-port <port>] [-size <size in KB>] "
                         "[-msgsize | -messagesize <size_in_bytes>] [-iterations <iterations>] [-concurrentFlows <flows>] "
                         "[-nolog|-nologging] [-enableMocketKeyExchange]\n", argv[0]);
        return -1;
    }

    if ((!bMockets) && (!bSockets)) {
        // Neither -mockets nor -sockets was specified - assume both
        bMockets = true;
        bSockets = true;
    }
    if (bServer) {
        // On a server - start up both MessageMocket and socket
        if (!bNoLogging) {
            pLogger = new Logger();
            pLogger->initLogFile ("intDataTest-server.log", false);
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();
        }

        FILE *fileLog = fopen (Constants::mocketsServerStatsFile.c_str(), "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file %s\n", Constants::mocketsServerStatsFile.c_str());
            return -2;
        }
        fprintf (fileLog, "Delta Time, Time, SentPacketsCount, SentByteCount, ReceivedPacketsCount, ReceivedBytesCount, Retransmits, DuplicateDiscarded, NoRoomDiscarded\n");
        fclose (fileLog);

        fileLog = fopen (Constants::socketsServerStatsFile.c_str(), "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file %s\n", Constants::socketsServerStatsFile.c_str ());
            return -2;
        }
        fprintf (fileLog, "Delta Time, Time\n");
        fclose (fileLog);

        printf ("Creating a ServerMocket and ServerSocket on port %d\n", (int) ui16Port);
        ServerMocketThread pSMT(ui16Port);
        pSMT.start();
        ServerSocketThread pSST(ui16Port);
        // Call run() instead of start() so that the main thread does not end
        pSST.run();
    }
    else if (bClient) {
        if (!bNoLogging) {
            pLogger = new Logger();
            pLogger->initLogFile ("intDataTest-client.log", false);
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();
        }

        if (bMockets) {
            FILE *fileLog = fopen (Constants::mocketsClientStatsFile.c_str(), "w");
            if (fileLog == NULL) {
                fprintf (stderr, "failed to create file %s\n", Constants::mocketsClientStatsFile.c_str());
                return -3;
            }
            fprintf (fileLog, "Delta Time, Time, SentPacketsCount, SentByteCount, ReceivedPacketsCount, ReceivedBytesCount, Retransmits, DuplicateDiscarded, NoRoomDiscarded, Throughput (KBps)\n");
            fclose (fileLog);
        }
        if (bSockets) {
            FILE *socfile = fopen (Constants::socketsClientStatsFile.c_str(), "w");
            if (socfile == NULL) {
                fprintf (stderr, "failed to create file %s\n", Constants::socketsClientStatsFile.c_str());
                return -3;
            }
            fprintf (socfile, "Delta Time, Time, Throughput (KBps)\n");
            fclose (socfile);
        }

        printf ("Client: Data size: %u KB\n", ui32SizeInKB);
        printf ("Client: Message size: %hu Bytes\n", (int) ui16MessageSizeInBytes);
        printf ("Client: number of iterations: %hu\n", (int) ui16Iterations);
        printf ("Client: will connect to host %s on port %hu\n\n", (const char*) remoteHost, (int) ui16Port);

        Statistics mocketStats, socketStats;
        AtomicVar<double> dSocketThroughput(0.0), dMocketThroughput(0.0);
        const double dTotalThroughputPerIterationInKB = ui16ConcurrentFlows * ui32SizeInKB * (1000.0L / 1024.0L);
        for (int i = 0; i < (int) ui16Iterations; i++) {
            static int64 ui64OverallSocketStartTime = 0, ui64OverallSocketEndTime = 0;
            static int64 ui64OverallMocketStartTime = 0, ui64OverallMocketEndTime = 0;
            printf ("Client: Iteration number %d\n", i);

            if (bSockets) {
                vector<ClientTaskThread> threadVector;
                // Create threads
                for (int j = 0; j < ui16ConcurrentFlows; j++) {
                    threadVector.push_back (ClientTaskThread ((i * ui16ConcurrentFlows) + j, remoteHost, ui16Port, false, bEnableKeyExchange,
                                                              ui32SizeInKB, ui16MessageSizeInBytes, &socketStats, dSocketThroughput));
                }
                
                printf ("Starting Socket Test...\n");
                ui64OverallSocketStartTime = getTimeInMilliseconds();
                // Start threads
                for (vector<ClientTaskThread>::iterator iter = threadVector.begin(); iter != threadVector.end (); iter++) {
                    (*iter).start(false);
                }
                // Join threads before moving on to the next iteration
                for (vector<ClientTaskThread>::iterator iter = threadVector.begin(); iter != threadVector.end (); iter++) {
                    (*iter).join();
                }
                ui64OverallSocketEndTime = getTimeInMilliseconds();
                printf ("\n");

                if (bMockets) {
                    printf ("Done with Socket Tests - Sleeping for %d seconds before starting Mocket Test...\n\n", SECS_TO_SLEEP);
                    sleepForMilliseconds (SECS_TO_SLEEP * 1000);
                }
            }
            if (bMockets) {
                // Create threads
                vector<ClientTaskThread> threadVector;
                for (int j = 0; j < ui16ConcurrentFlows; j++) {
                    threadVector.push_back (ClientTaskThread ((i * ui16ConcurrentFlows) + j, remoteHost, ui16Port, true, bEnableKeyExchange,
                                                              ui32SizeInKB, ui16MessageSizeInBytes, &mocketStats, dMocketThroughput));
                }

                printf ("Starting Mocket Test...\n");
                ui64OverallMocketStartTime = getTimeInMilliseconds();
                // Start threads
                for (vector<ClientTaskThread>::iterator iter = threadVector.begin(); iter != threadVector.end (); iter++) {
                    (*iter).start(false);
                }
                // Join threads before moving on to the next iteration
                for (vector<ClientTaskThread>::iterator iter = threadVector.begin(); iter != threadVector.end (); iter++) {
                    (*iter).join();
                }
                ui64OverallMocketEndTime = getTimeInMilliseconds();
                printf ("\n");
            }

            printf ("-----------------------------------------\n");
            printf ("Total number of experiments: %d\n", (bSockets + bMockets) * ui16ConcurrentFlows);
            if (bSockets) {
                if (ui16ConcurrentFlows > 1) {
                    printf ("Socket Iteration Throughput:    %10.4f\n", dTotalThroughputPerIterationInKB /
                                                                        (ui64OverallSocketEndTime - ui64OverallSocketStartTime));
                }
                printf ("Socket Stats:: Average:       %10.4f\n", socketStats.getAverage());
                if (i > 0) {
                    printf ("Socket Stats:: St Deviation:  %10.4f\n", socketStats.getStDev());
                }
            }
            if (bMockets) {
                if (ui16ConcurrentFlows > 1) {
                    printf ("Mocket Iteration Throughput:    %10.4f\n", dTotalThroughputPerIterationInKB /
                                                                        (ui64OverallMocketEndTime - ui64OverallMocketStartTime));
                }
                printf ("Mocket Stats:: Average:       %10.4f\n", mocketStats.getAverage());
                if (i > 0) {
                    printf ("Mocket Stats:: St Deviation:  %10.4f\n", mocketStats.getStDev());
                }
            }
            printf ("-----------------------------------------\n");
            if (i != (int) (ui16Iterations - 1)) {
                printf ("Sleeping for %d seconds before next iteration...\n", SECS_TO_SLEEP);
                sleepForMilliseconds (SECS_TO_SLEEP * 1000);
            }
        }

        printf ("\n\t***** TEST FINISHED *****\n");
        if ((ui16Iterations > 1) || (ui16ConcurrentFlows > 1)) {
            double dSpeedKBytesPerMS;
            if (bSockets) {
                dSpeedKBytesPerMS = dSocketThroughput / (ui16Iterations * ui16ConcurrentFlows);
                printf ("Average Socket Throughput = %.2f KB/s\n", dSpeedKBytesPerMS);
            }
            if (bMockets) {
                dSpeedKBytesPerMS = dMocketThroughput / (ui16Iterations * ui16ConcurrentFlows);
                printf ("Average Mocket Throughput = %.2f KB/s\n", dSpeedKBytesPerMS);
            }
        }
    }

    delete pLogger;
    pLogger = NULL;

    #if defined (WIN32) && defined (_DEBUG)
        _CrtDumpMemoryLeaks();
        // getchar(); // Useful when running the program in a debugger and the output window should stay open
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
