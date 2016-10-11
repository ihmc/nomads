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
#include "SCTPSocket.h"
#include "udt/udt.h"
#include "Thread.h"
#include "Statistics.h"
#include <netinet/in.h>
#include <netinet/sctp.h>

/**
    In the IntDataTest2 class the server sends the data and the client
    receives it and sends the acknoledgement.
**/

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
#define DEFAULT_MESSAGE_SIZE_IN_BYTES 1400
#define SECS_TO_SLEEP 5

using namespace std;
using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
double doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, const char* pProtocol, bool bEnableKeyExchange, uint32 ui32SizeInKB, uint16 ui16MessageSizeInBytes, Statistics *pStats);

class Constants
{
public:
    static const int32 i32UDTListenNum;
    static const uint64 ui64SecsToSleep;
    static const String socketsServerStatsFile;
    static const String socketsClientStatsFile;
    static const String mocketsServerStatsFile;
    static const String mocketsClientStatsFile;
    static const String sctpSocketServerStatsFile;
    static const String sctpSocketClientStatsFile;
    static const String udtServerStatsFile;
    static const String udtClientStatsFile;
    static AtomicVar<double> defaultStatsVar;
};

const uint64 ui64SecsToSleep (2);
const int32 i32UDTListenNum (10);
const String Constants::socketsServerStatsFile("stats-server-sockets-cpp.txt");
const String Constants::socketsClientStatsFile("stats-Client-sockets-cpp.txt");
const String Constants::mocketsServerStatsFile("stats-server-MsgMockets-cpp.txt");
const String Constants::mocketsClientStatsFile("stats-Client-MsgMockets-cpp.txt");
const String Constants::sctpSocketServerStatsFile("stats-server-sctp-cpp.txt");
const String Constants::sctpSocketClientStatsFile("stats-Client-sctpcpp.txt");
const String Constants::udtServerStatsFile("stats-Server-MsgUDT-cpp.txt");
const String Constants::udtClientStatsFile("stats-Client-MsgUDT-cpp.txt");
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

class SCTPServerSocketThread : public Thread
{
    public:
        SCTPServerSocketThread (int usServerPort);
        void run (void);

    private:
        SCTPSocket *_pSCTPServerSocket;
};

class ServerUDTThread : public Thread
{
    public:
        ServerUDTThread (int usServerPort);
        ~ServerUDTThread (void);

        void run (void);

    private:
        UDTSOCKET _udtServerSocket;
};


class ClientTaskThread : public Thread
{
public:
    ClientTaskThread (uint16 ui16ThreadID, const char * const pszRemoteHost, unsigned short usRemotePort, const char* pProtocol, bool bEnableKeyExchange,
                      uint32 ui32SizeInKB, uint16 ui16MessageSizeInBytes, Statistics * const pStats, AtomicVar<double> &avThroughputStats);
    ClientTaskThread (const ClientTaskThread &rhs);

    ClientTaskThread & operator= (const ClientTaskThread &rhs);

    void run(void);

private:
    uint16 _ui16ThreadID;
    const char *_pszRemoteHost;
    unsigned short _usRemotePort;
    bool _bUseMockets;
    bool _bUseSockets;
    bool _bUseSCTPSockets;
    bool _bUseUDTSockets;
    const char* _pProtocol;
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
	ConnHandler (SCTPSocket *pSCTPSocket);
        ConnHandler (UDTSOCKET udtSocket);
        void run (void);

    private:
        Mocket *_pMocket;
        Socket *_pSocket;
	SCTPSocket *_pSCTPSocket;
        UDTSOCKET _udtSocket;
        bool _useMockets;
        bool _useSockets;
        bool _useSCTPSockets;
        bool _useUDTSockets;
};

ServerMocketThread::ServerMocketThread (int usServerPort)
{
    const char szConfigFile[] = "mockets.conf";
    FILE *pConfigFile = fopen (szConfigFile, "r");
    if (pConfigFile != NULL) {
        fclose (pConfigFile);
        _pServerMocket = new ServerMocket (szConfigFile);
        printf ("Using config file %s\n", szConfigFile);
    }
    else {
        _pServerMocket = new ServerMocket();
    }
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

SCTPServerSocketThread::SCTPServerSocketThread (int usServerPort)
{
    _pSCTPServerSocket = new SCTPSocket();
    _pSCTPServerSocket->setupToReceive (usServerPort);
}

void SCTPServerSocketThread::run (void)
{
    while (true) {
        SCTPSocket *pSocket = _pSCTPServerSocket->accept();
        if (pSocket) {
            printf ("SCTPServerSocket: got a connection\n");
            ConnHandler *pHandler = new ConnHandler (pSocket);
            pHandler->start();
        }
    }
}

ServerUDTThread::ServerUDTThread (int usServerPort)
{
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = EndianHelper::htons (usServerPort);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(local_addr.sin_zero), '\0', sizeof (local_addr.sin_zero));

    _udtServerSocket = UDT::socket(AF_INET, SOCK_DGRAM, 0);
    if (UDT::ERROR == UDT::bind (_udtServerSocket, (sockaddr*)&local_addr, sizeof (local_addr))) {
        printf ("Error during bind of UDT socket: %s\n", UDT::getlasterror().getErrorMessage());
        return;
    }
    UDT::listen (_udtServerSocket, i32UDTListenNum);
}

ServerUDTThread::~ServerUDTThread (void)
{
    UDT::close (_udtServerSocket);
}

void ServerUDTThread::run (void)
{
    int namelen;
    sockaddr_in remote_addr;
    UDTSOCKET udtSocket = UDT::INVALID_SOCK;
    
    while (true) {
        if (UDT::INVALID_SOCK == (udtSocket = UDT::accept (_udtServerSocket, reinterpret_cast<sockaddr *> (&remote_addr), &namelen))) {
            printf ("UDTServerSocket: error encountered while accepting a new connection: %s\n", UDT::getlasterror().getErrorMessage());
            continue;
        }
        printf ("UDTServerSocket: got a connection from %s:%hu\n",
                //InetAddr(remote_addr.sin_addr.S_un.S_addr).getIPAsString(), EndianHelper::ntohs (remote_addr.sin_port));
                InetAddr(remote_addr.sin_addr.s_addr).getIPAsString(), EndianHelper::ntohs (remote_addr.sin_port));
        ConnHandler *pHandler = new ConnHandler (udtSocket);
        pHandler->start();
    }
}

ConnHandler::ConnHandler (Mocket *pMocket)
{
    _pMocket = pMocket;
    _pSocket = NULL;
    _pSCTPSocket = NULL;
    _udtSocket = UDT::INVALID_SOCK;
    _useMockets = true;
    _useSockets = false;
    _useSCTPSockets = false;
    _useUDTSockets = false;
}

ConnHandler::ConnHandler (Socket *pSocket)
{
    _pMocket = NULL;
    _pSCTPSocket = NULL;
    _udtSocket = UDT::INVALID_SOCK;
    _useSockets = true;
    _pSocket = pSocket;
    _useMockets = false;
    _useSCTPSockets = false;
    _useUDTSockets = false;
}

ConnHandler::ConnHandler (SCTPSocket *pSCTPSocket)
{
    _pSocket = NULL;
    _pMocket = NULL;
    _udtSocket = UDT::INVALID_SOCK;
    _pSCTPSocket = pSCTPSocket;
    _useSCTPSockets = true;
    _useMockets = false;
    _useSockets = false;
    _useUDTSockets = false;
}

ConnHandler::ConnHandler (UDTSOCKET udtSocket)
{
    _pSocket = NULL;
    _pMocket = NULL;
    _pSCTPSocket = NULL;
    _udtSocket = udtSocket;
    _useUDTSockets = true;
    _useMockets = false;
    _useSockets = false;
    _useSCTPSockets = false;
    
}

void ConnHandler::run (void)
{
    int rc;
    int64 i64StartTime, i64EndTime;
    unsigned int iTime;

    uint32 ui32SizeInKB =  DEFAULT_TOTAL_SIZE_IN_KB;

    int i;
    char chReply = 0;
    int iBytesRead = 0, iBytesToRead = 0;
    int iBytesSent = 0, iBytesToSend = 0;

    //unsigned char *buf = new unsigned char[iBytesToRead];
    unsigned char *buf = new unsigned char[MAX_MESSAGE_SIZE];

    // Fill the buffer with random data
    for (int i = 0; i < MAX_MESSAGE_SIZE; i++) {
        buf[i] = (char) rand();
    }

    if (_useMockets) {
        printf ("***Server Protocol: mockets\n");
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

        uint16 ui16MessageSizeInBytes = DEFAULT_MESSAGE_SIZE_IN_BYTES;

        i64StartTime = getTimeInMilliseconds();

        while (iBytesSent < iBytesToRead) {
            #if defined (UNIX)
            iBytesToSend = std::min ((int) ui16MessageSizeInBytes, (int)(iBytesToRead - iBytesSent));
            #elif defined (WIN32)
            iBytesToSend = min ((int) ui16MessageSizeInBytes, (ui32DataSize - iBytesSent));
            #endif
            _pMocket->send (true, true, buf, iBytesToSend, 0, 5, 0, 0);
            iBytesSent += iBytesToSend;
        }

        rc = _pMocket->receive (&chReply, 1);

        i64EndTime = getTimeInMilliseconds();

        _pMocket->close();

        if (chReply != '.') {
            logErrorToLogFile (Constants::mocketsClientStatsFile.c_str(), rc);
            fprintf (stderr, "doClientTask: failed to receive '.' from remote host\n");
            delete[] buf;
            return;
        }

        iTime = (unsigned int) (i64EndTime - i64StartTime);

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
    else if (_useSockets) {
        printf ("***Using Server Protocol: sockets\n");
        printf ("ConnHandler::run: client handler thread started for an incoming sockets connection\n");
        if (sizeof (iBytesToRead) != _pSocket->receive (&iBytesToRead, sizeof (iBytesToRead))) {
            printf ("ConnHandler::run: receive failed to read size of data; terminating\n");
            _pSocket->disconnect();
            delete _pSocket;
            return;
        }
        iBytesToRead = EndianHelper::ntohl ((uint32)iBytesToRead);
        printf ("ConnHandler::run: will read [%d] bytes from client\n", iBytesToRead);

        int iBytesSent = 0, iBytesToSend = 0;
        uint16 ui16MessageSizeInBytes = DEFAULT_MESSAGE_SIZE_IN_BYTES;

        i64StartTime = getTimeInMilliseconds();

        while (iBytesSent < iBytesToRead) {
            #if defined (UNIX)
            iBytesToSend = std::min ((int) ui16MessageSizeInBytes, (int)(iBytesToRead - iBytesSent));
            #elif defined (WIN32)  
            iBytesToSend = min ((int) ui16MessageSizeInBytes, (iBytesToRead - iBytesSent));
            #endif
            _pSocket->send (buf, iBytesToSend);
            iBytesSent += iBytesToSend;
        }

        _pSocket->receive (&chReply, 1);
        i64EndTime = getTimeInMilliseconds();

        _pSocket->disconnect();

        if (chReply != '.') {
            fprintf (stderr, "doClientTask: failed to receive '.' from remote host\n");
            delete[] buf;
            return;
        }

        iTime = (unsigned int) (i64EndTime - i64StartTime);

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
    else if (_useSCTPSockets) {
        printf ("ConnHandler::run: client handler thread started for an incoming sctp sockets connection\n");
        if (sizeof (iBytesToRead) != _pSCTPSocket->receive (&iBytesToRead, sizeof (iBytesToRead))) {
            printf ("ConnHandler::run: receive failed to read size of data; terminating\n");
            _pSCTPSocket->disconnect();
            delete _pSCTPSocket;
            return;
        }
        iBytesToRead = EndianHelper::ntohl ((uint32)iBytesToRead);
        printf ("ConnHandler::run: will read [%d] bytes from client\n", iBytesToRead);

        int iBytesSent = 0, iBytesToSend = 0;
        uint16 ui16MessageSizeInBytes = DEFAULT_MESSAGE_SIZE_IN_BYTES;

        i64StartTime = getTimeInMilliseconds();

        while (iBytesSent < iBytesToRead) {
            #if defined (UNIX)
            iBytesToSend = std::min ((int) ui16MessageSizeInBytes, (int)(iBytesToRead - iBytesSent));
            #elif defined (WIN32)
            iBytesToSend = min ((int) ui16MessageSizeInBytes, (iBytesToRead - iBytesSent));
            #endif
            _pSCTPSocket->send (buf, iBytesToSend);
            iBytesSent += iBytesToSend;
        }
        _pSCTPSocket->receive (&chReply, 1);

        i64EndTime = getTimeInMilliseconds();
 
        _pSCTPSocket->disconnect();

        if (chReply != '.') {
            fprintf (stderr, "doClientTask: failed to receive '.' from remote host\n");
            delete[] buf;
            return;
        }

        iTime = (unsigned int) (i64EndTime - i64StartTime);

        if (iTime == 0) {
            iTime = 1;
        }

        // Save results to a file
        FILE *socfile = fopen(Constants::sctpSocketServerStatsFile.c_str(), "a");
        if (socfile == NULL) {
            fprintf(stderr, "failed to append to file %s\n", Constants::sctpSocketServerStatsFile.c_str());
            return;
        }
        fprintf (socfile, "%lu, %d\n", (unsigned long) (getTimeInMilliseconds()/1000), iTime);
        fclose (socfile);
    }
    else if (_useUDTSockets) {
        // UDT
        printf ("ConnHandler::run: client handler thread started for an incoming UDT connection\n");

        if (UDT::ERROR == (i = UDT::recvmsg (_udtSocket, reinterpret_cast<char *> (&iBytesToRead), sizeof (iBytesToRead)))) {
            printf ("Error in UDT receiving the size of the data to read: %s\n", UDT::getlasterror().getErrorMessage());
            UDT::close (_udtSocket);
            return;
        }

        iBytesToRead = EndianHelper::ntohl ((uint32) iBytesToRead);

        printf ("ConnHandler::run: will read [%d] bytes from client\n", iBytesToRead);

        int iBytesSent = 0, iBytesToSend = 0;
        uint16 ui16MessageSizeInBytes = DEFAULT_MESSAGE_SIZE_IN_BYTES;

        i64StartTime = getTimeInMilliseconds();
        while (iBytesSent < iBytesToRead) {
            #if defined (UNIX)
            iBytesToSend = std::min ((int) ui16MessageSizeInBytes, (int)(iBytesToRead - iBytesSent));
            #elif defined (WIN32)
            iBytesToSend = min ((int) ui16MessageSizeInBytes, (iBytesToRead - iBytesSent));
            #endif
            if (UDT::ERROR == UDT::sendmsg (_udtSocket, reinterpret_cast<const char *> (buf), iBytesToSend, -1, true)) {
                fprintf (stderr, "doClientTask: an error occurred after %d bytes were sent; %s\n", iBytesSent, UDT::getlasterror().getErrorMessage());
                UDT::close (_udtSocket);
                delete[] buf;
                return;
            }
            iBytesSent += iBytesToSend;
        }

        if (UDT::ERROR == (rc = UDT::recvmsg (_udtSocket, &chReply, sizeof (chReply)))) {
            printf ("doClientTask: an error occurred while receiving the final '.'; %s\n", UDT::getlasterror().getErrorMessage());
            UDT::close (_udtSocket);
            delete[] buf;
            return;
        }

        iTime = (unsigned int) (getTimeInMilliseconds() - i64StartTime);

        if (iTime == 0) {
            iTime = 1;
        }

        // Save results to a file
        FILE *file = fopen(Constants::udtServerStatsFile.c_str(), "a");
        if (file == NULL) {
            fprintf(stderr, "ConnHandler::run: failed to append to file %s\n",
                    Constants::udtServerStatsFile.c_str());
            return;
        }

        UDT::TRACEINFO traceInfo;
        if (UDT::ERROR == UDT::perfmon (_udtSocket, &traceInfo)) {
            fprintf (stderr, "ConnHandler::run: an error occurred with UDT while calling perfmon()\n");
        }
        else {
            fprintf (file, "%lu, %d, %lu, %lu, %d, %d, %d\n", (unsigned long) (getTimeInMilliseconds () / 1000),
                     iTime, traceInfo.pktSentTotal, traceInfo.pktRecvTotal, traceInfo.pktRetransTotal,
                     traceInfo.pktSndLossTotal, traceInfo.pktRcvLossTotal);
        }
        fclose (file);
        UDT::close (_udtSocket);
    }

    printf ("ConnHandler::run: client handler thread finished\n");
}

ClientTaskThread::ClientTaskThread (uint16 ui16ThreadID, const char * const pszRemoteHost, unsigned short usRemotePort, const char* pProtocol, bool bEnableKeyExchange, uint32 ui32SizeInKB, uint16 ui16MessageSizeInBytes, Statistics * const pStats, AtomicVar<double> &avThroughputStats) :
    _ui16ThreadID(ui16ThreadID), _pszRemoteHost(pszRemoteHost), _usRemotePort(usRemotePort), _pProtocol(pProtocol), _bEnableKeyExchange(bEnableKeyExchange), _ui32SizeInKB(ui32SizeInKB), _ui16MessageSizeInBytes(ui16MessageSizeInBytes), _pStats(pStats), _avThroughputStats(avThroughputStats) {}

ClientTaskThread::ClientTaskThread (const ClientTaskThread &rhs) :
    _ui16ThreadID(rhs._ui16ThreadID), _pszRemoteHost(rhs._pszRemoteHost), _usRemotePort(rhs._usRemotePort), _pProtocol(rhs._pProtocol),
    _bEnableKeyExchange(rhs._bEnableKeyExchange), _ui32SizeInKB(rhs._ui32SizeInKB), _ui16MessageSizeInBytes(rhs._ui16MessageSizeInBytes),
    _pStats(rhs._pStats), _avThroughputStats(rhs._avThroughputStats) {}

ClientTaskThread & ClientTaskThread::operator= (const ClientTaskThread &rhs)
{
    _ui16ThreadID = rhs._ui16ThreadID;
    _pszRemoteHost = rhs._pszRemoteHost;
    _usRemotePort = rhs._usRemotePort;
    _pProtocol = rhs._pProtocol;
    _bEnableKeyExchange = rhs._bEnableKeyExchange;
    _ui32SizeInKB = rhs._ui32SizeInKB;
    _ui16MessageSizeInBytes = rhs._ui16MessageSizeInBytes;
    Statistics *_pStats = rhs._pStats;
    _avThroughputStats = rhs._avThroughputStats;

    return (*this);
}

void ClientTaskThread::run (void)
{
    double tp = doClientTask(_pszRemoteHost, _usRemotePort, _pProtocol, _bEnableKeyExchange, _ui32SizeInKB, _ui16MessageSizeInBytes, _pStats);
    printf ("***ClientTaskThread:Protocol: %s\n", _pProtocol);
    if (tp < 0) {
        if (0 == stricmp (_pProtocol, "mockets")) {
           fprintf(stderr, "Thread# %hu: client task using %s has failed\n", _ui16ThreadID, _pProtocol);
        }
        if (0 == stricmp (_pProtocol, "sockets")) {
           fprintf(stderr, "Thread# %hu: client task using %s has failed\n", _ui16ThreadID, _pProtocol);
        }
        if (0 == stricmp (_pProtocol, "sctpSockets")) {
           fprintf(stderr, "Thread# %hu: client task using %s has failed\n", _ui16ThreadID, _pProtocol);
        }
        if (0 == stricmp (_pProtocol, "udt")) {
           fprintf(stderr, "Thread# %hu: client task using %s has failed\n", _ui16ThreadID, _pProtocol);
        }
        //fprintf(stderr, "Thread# %hu: client task using %s has failed\n", _ui16ThreadID, _bUseMockets ? "Mockets" : "Sockets");
        return;
    }

    _avThroughputStats += tp;
}

double doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, const char *pProtocol, bool bEnableKeyExchange, uint32 ui32SizeInKB, uint16 ui16MessageSizeInBytes, Statistics *pStats)
{
    int i;
    static const char chReply = '.';

    int iBytesRead = 0;
    unsigned int iTime = 0;

    int rc;
    double dThroughput;
    int64 i64StartTime, i64EndTime;


    static char buf[MAX_MESSAGE_SIZE];

    srand ((unsigned int) getTimeInMilliseconds());

    // Following two locks are used to avoid race conditions on Statistics *pStats.
    static Mutex socketsStatsMutex;
    static Mutex mocketsStatsMutex;
    static Mutex sctpSocketsStatsMutex;
    static Mutex udtStatsMutex;

    if (0 == stricmp (pProtocol, "mockets")) {
        printf ("***Protocol: mockets\n");
        Mocket pm;
        pm.setIdentifier ("IntDataTest-Client");
        pm.registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
        const char szConfigFile[] = "mockets.conf";
        FILE *pConfigFile = fopen (szConfigFile, "r");
        if (pConfigFile != NULL) {
            fclose (pConfigFile);
            pm.readConfigFile (szConfigFile);
            printf ("Read Mockets config file %s\n", szConfigFile);
        }
        if (0 != (rc = pm.connect (pszRemoteHost, usRemotePort, bEnableKeyExchange))) {
            fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            printf ("doClientTask: Unable to connect\n");
            return -1;
        }

        uint32 ui32DataSize = ui32SizeInKB * 1024;
        int iDataSizeInNBO = EndianHelper::htonl (ui32DataSize);
        
        i64StartTime = getTimeInMilliseconds();

        MessageSender sender = pm.getSender (true, true);
        sender.send (&iDataSizeInNBO, sizeof (iDataSizeInNBO));

        while (iBytesRead < ui32DataSize) {
            if ((i = pm.receive (buf, sizeof (buf))) > 0) {
                iBytesRead += i;
            }
            else {
                printf ("ConnHandler::run: receive returned %d; closing connection\n", i);
                break;
            }
        }

        i64EndTime = getTimeInMilliseconds();
     
        iTime = (unsigned int) (i64EndTime - i64StartTime);

        sender.send (&chReply, sizeof (chReply));
        
       
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
    if (0 == stricmp (pProtocol, "sockets")) {
        printf ("***Protocol: sockets\n");
        TCPSocket socket;
        if (0 != (rc = socket.connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using sockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            return -5;
        }

        uint32 ui32DataSize = ui32SizeInKB * 1024;
        int iDataSizeInNBO = EndianHelper::htonl (ui32DataSize);
        
        i64StartTime = getTimeInMilliseconds();
       
        socket.send (&iDataSizeInNBO, sizeof (iDataSizeInNBO));

        while (iBytesRead < ui32DataSize) {
            if ((i = socket.receive (buf, sizeof (buf))) > 0) {
                iBytesRead += i;
            }
            else {
                printf ("ConnHandler::run: receive returned %d; closing connection\n", i);
                break;
            }
        }

        i64EndTime = getTimeInMilliseconds();    
	iTime = (unsigned int) (i64EndTime - i64StartTime);

        socket.send (&chReply, sizeof (chReply));
        socket.disconnect();

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
            return -8;
        }
        fprintf (socfile, "%lu, %d, %.2f\n", (unsigned long) (getTimeInMilliseconds() / 1000), iTime, dThroughput);
        fclose (socfile);

        socketsStatsMutex.unlock();
    }

    if (0 == stricmp (pProtocol, "sctpSockets")) {
        printf ("***Protocol: sctpSockets\n");
        SCTPSocket socket;

        if (0 != (rc = socket.connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using sctpSockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            return -5;
        }

        uint32 ui32DataSize = ui32SizeInKB * 1024;
        int iDataSizeInNBO = EndianHelper::htonl (ui32DataSize);

        i64StartTime = getTimeInMilliseconds();
        
        socket.send (&iDataSizeInNBO, sizeof (iDataSizeInNBO));

        while (iBytesRead < ui32DataSize) {
            if ((i = socket.receive (buf, sizeof (buf))) > 0) {
                iBytesRead += i;
            }
            else {
                printf ("ConnHandler::run: receive returned %d; closing connection\n", i);
                break;
            }
        }

        i64EndTime = getTimeInMilliseconds();
     
        iTime = (unsigned int) (i64EndTime - i64StartTime);

        socket.send (&chReply, sizeof (chReply));
        socket.disconnect();

        printf ("ConnHandler::run: read a total of [%d] bytes; sending ACK\n", iBytesRead);

        if (iTime == 0) {
	    iTime = 1;
	}

        sctpSocketsStatsMutex.lock();

        printf ("DataSize = %d bytes = %d Kbytes\n", ui32DataSize, ui32SizeInKB);
        printf ("Time = %dms\n", iTime);
        dThroughput = ((double) ui32DataSize) / iTime;
        printf ("Throughput (bytes/ms) = %.2f (%d/%d)\n", dThroughput, ui32DataSize, iTime);
        dThroughput = (dThroughput * 1000) / 1024;
        printf ("Throughput (Kbytes/s) = %.2f (%d/%.2f)\n", dThroughput, ui32SizeInKB, ((double) iTime) / 1000);

        pStats->update ((double) (i64EndTime - i64StartTime));

        // Save results to a file
        FILE *socfile = fopen (Constants::sctpSocketClientStatsFile.c_str(), "a");
        if (socfile == NULL) {
            fprintf (stderr, "failed to append to file %s\n", Constants::sctpSocketClientStatsFile.c_str());
            return -8;
        }
        fprintf (socfile, "%lu, %d, %.2f\n", (unsigned long) (getTimeInMilliseconds() / 1000), iTime, dThroughput);
        fclose (socfile);

        sctpSocketsStatsMutex.unlock();
    }

    if (0 == stricmp (pProtocol, "udt")) {
        // UDT
        sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(usRemotePort);
        #if defined (UNIX)
            inet_pton(AF_INET, pszRemoteHost, &serv_addr.sin_addr);
        #elif defined (WIN32)
            inet_pton(AF_INET, pszRemoteHost, &serv_addr.sin_addr);
        #endif
        memset(&(serv_addr.sin_zero), '\0', sizeof (serv_addr.sin_zero));

        UDTSOCKET udtClientSocket = UDT::socket (AF_INET, SOCK_DGRAM, 0);

        uint32 ui32DataSize = ui32SizeInKB * 1024;
        int iDataSizeInNBO = EndianHelper::htonl (ui32DataSize);

        // Connect to the server, implict bind
        if (UDT::ERROR == UDT::connect (udtClientSocket, reinterpret_cast<sockaddr*> (&serv_addr), sizeof (serv_addr))) {
            fprintf (stderr, "doClientTask: failed to connect using UDT to remote host %s on port %d; %s\n",
                     pszRemoteHost, usRemotePort, UDT::getlasterror().getErrorMessage());
            UDT::close (udtClientSocket);
            return -9;
        }

        i64StartTime = getTimeInMilliseconds();
 
       if (UDT::ERROR == UDT::sendmsg (udtClientSocket, reinterpret_cast<const char *> (&iDataSizeInNBO), sizeof (iDataSizeInNBO), -1, true)) {
            fprintf (stderr, "doClientTask: failed to send the transfer size; %s\n", UDT::getlasterror().getErrorMessage());
            UDT::close (udtClientSocket);
            return -10;
        }
        
        while (iBytesRead < ui32DataSize) {
            if (UDT::ERROR == (i = UDT::recvmsg (udtClientSocket, buf, sizeof (buf)))) {
                printf ("Error in UDT receiving the size of the data to read: %s\n", UDT::getlasterror().getErrorMessage());
                break;
            }
            iBytesRead += i;
        }

        i64EndTime = getTimeInMilliseconds();

        iTime = (unsigned int) (i64EndTime - i64StartTime);
        
        if (UDT::ERROR == (i = UDT::sendmsg (udtClientSocket, &chReply, sizeof (chReply), -1, true))) {
            printf ("ConnHandler::run: an error occurred with UDT while receiving the size of the data to read: %s\n",
                    UDT::getlasterror().getErrorMessage());
        }

        printf ("ConnHandler::run: read a total of [%d] bytes; sent ACK\n", iBytesRead);
        
        if (iTime == 0) {
	    iTime = 1;
	}

        udtStatsMutex.lock();
        printf ("DataSize = %d bytes = %d Kbytes\n", ui32DataSize, ui32SizeInKB);
        printf ("Time = %dms\n", iTime);
        dThroughput = ((double) ui32DataSize) / iTime;
        printf ("Throughput (bytes/ms) = %.2f (%d/%d)\n", dThroughput, ui32DataSize, iTime);
        dThroughput = (dThroughput * 1000) / 1024;
        printf ("Throughput (Kbytes/s) = %.2f (%d/%.2f)\n", dThroughput, ui32SizeInKB, ((double) iTime) / 1000);

        pStats->update ((double) (i64EndTime - i64StartTime));

        // Save results to a file
        FILE *file = fopen (Constants::udtClientStatsFile.c_str(), "a");
        if (file == NULL) {
            fprintf (stderr, "doClientTask: failed to append to file %s\n", Constants::udtClientStatsFile.c_str());
            return -15;
        }

        UDT::TRACEINFO traceInfo;
        if (UDT::ERROR == UDT::perfmon (udtClientSocket, &traceInfo)) {
            fprintf (stderr, "doClientTask: an error occurred with UDT while calling perfmon()\n");
        }
        else {
            fprintf (file, "%lu, %d, %.2f, %lu, %lu, %d, %d, %d\n", (unsigned long) (getTimeInMilliseconds () / 1000),
                     iTime, dThroughput, traceInfo.pktSentTotal, traceInfo.pktRecvTotal, traceInfo.pktRetransTotal,
                     traceInfo.pktSndLossTotal, traceInfo.pktRcvLossTotal);
        }
        fclose (file);
        UDT::close (udtClientSocket);

        udtStatsMutex.unlock();
    }

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
    bool bSCTPSockets = false;
    bool bUDT = false;
    bool bLogging = false;
    uint16 ui16Port = 1234;
    uint16 ui16SecToSleep = SECS_TO_SLEEP;
    uint16 ui16Iterations = 10;
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
        else if ((0 == stricmp (argv[i], "-sctpSockets")) || (0 == stricmp (argv[i], "-sctp"))) {
            bSCTPSockets = true;
        }
        else if (0 == stricmp (argv[i], "-udt")) {
            bUDT = true;
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
        else if (0 == stricmp (argv[i], "-secToSleep")) {
            i++;
            if (i < argc) {
                ui16SecToSleep = (uint16) atoi (argv[i]);
            }
        }
        else if ((0 == stricmp (argv[i], "-log")) || (0 == stricmp (argv[i], "-logging"))) {
            bLogging = true;
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
        fprintf (stderr, "usage: %s {-server | -client -remotehost <remotehost>} [-mockets] [-sockets] [-sctpSockets|-sctp] [-udt] [-port <port>] [-size <size in KB>] "
                         "[-msgsize | -messagesize <size_in_bytes>] [-secToSleep <number of seconds>] [-iterations <iterations>] [-concurrentFlows <flows>] "
                         "[-log|-logging] [-enableMocketKeyExchange]\n", argv[0]);
        return -1;
    }

    if ((!bMockets) && (!bSockets) && (!bSCTPSockets) && !bUDT) {
        // No protocol was specified - assume all
        printf ("****** Enabling all protocols\n");
        bMockets = true;
        bSockets = true;
        bSCTPSockets = true;
        bUDT = true;
    }
    if ((bMockets) && (!bSockets) && (!bSCTPSockets) && !bUDT) {
        printf ("****** Enabling mockets protocol\n");
        bMockets = true;
        bSockets = false;
        bSCTPSockets = false;
        bUDT = false;
    }
    if ((!bMockets) && (bSockets) && (!bSCTPSockets) && !bUDT) {
        printf ("****** Enabling sockets protocol\n");
        bMockets = false;
        bSockets = true;
        bSCTPSockets = false;
        bUDT = false;
    }
    if ((!bMockets) && (!bSockets) && (bSCTPSockets) && !bUDT) {
        printf ("****** Enabling sctp protocol\n");
        bMockets = false;
        bSockets = false;
        bSCTPSockets = true;
        bUDT = false;
    }
    if ((!bMockets) && (!bSockets) && (!bSCTPSockets) && bUDT) {
        printf ("****** Enabling udt protocol\n");
        bMockets = false;
        bSockets = false;
        bSCTPSockets = false;
        bUDT = true;
    }
    if (bServer) {
        // On a server - start up MessageMocket, socket and SCTPsocket
        if (bLogging) {
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
            return -3;
        }
        fprintf (fileLog, "Delta Time, Time\n");
        fclose (fileLog);

        fileLog = fopen (Constants::sctpSocketServerStatsFile.c_str(), "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file %s\n", Constants::sctpSocketServerStatsFile.c_str ());
            return -4;
        }
        fprintf (fileLog, "Delta Time, Time\n");
        fclose (fileLog);

        fileLog = fopen (Constants::udtServerStatsFile.c_str(), "w");
        if (fileLog == NULL) {
            fprintf (stderr, "failed to write to file %s\n", Constants::udtServerStatsFile.c_str ());
            return -5;
        }
        fprintf (fileLog, "Delta Time, Time, SentPacketsCount, ReceivedPacketsCount, Retransmits, SendLoss, ReceiveLoss\n");
        fclose (fileLog);

       if ((bMockets) && (bSockets) && (bSCTPSockets) && bUDT) {
            printf ("Creating a ServerMocket, ServerSocket and SCTPServer on port %hu, and a UDTServerSocket on port %hu\n", ui16Port, ui16Port + 2);
	    ServerMocketThread pSMT(ui16Port);
	    pSMT.start();

	    ServerSocketThread pSST(ui16Port);
	    pSST.start();

	    SCTPServerSocketThread pSSST(ui16Port);
	    pSSST.start();

            ServerUDTThread pSUDT (ui16Port + 2);
            // Call run() instead of start() so that the main thread does not end
            pSUDT.run();
	}
        if ((bMockets) && (!bSockets) && (!bSCTPSockets) && !bUDT) {
		ServerMocketThread pSMT(ui16Port);
		pSMT.run();

	}
       if ((!bMockets) && (bSockets) && (!bSCTPSockets) && !bUDT) {
		ServerSocketThread pSST(ui16Port);
		// Call run() instead of start() so that the main thread does not end
		pSST.run();

	}
	if ((!bMockets) && (!bSockets) && (bSCTPSockets) && !bUDT) {
		SCTPServerSocketThread pSSST(ui16Port);
		pSSST.run();

	 }
         
	if ((!bMockets) && (!bSockets) && (!bSCTPSockets) && bUDT) {
	    ServerUDTThread pSUDT (ui16Port + 2);
            pSUDT.run();

	 }
    }
    else if (bClient) {
        if (bLogging) {
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
        if (bSCTPSockets) {
            FILE *socfile = fopen (Constants::sctpSocketClientStatsFile.c_str(), "w");
            if (socfile == NULL) {
                fprintf (stderr, "failed to create file %s\n", Constants::sctpSocketClientStatsFile.c_str());
                return -3;
            }
            fprintf (socfile, "Delta Time, Time, Throughput (KBps)\n");
            fclose (socfile);
        }

       if (bUDT) {
            FILE *file = fopen (Constants::udtClientStatsFile.c_str(), "w");
            if (file == NULL) {
                fprintf (stderr, "failed to create file %s\n", Constants::udtClientStatsFile.c_str());
                return -3;
            }
            fprintf (file, "Delta Time, Time, Throughput (KBps), SentPacketsCount, ReceivedPacketsCount, Retransmits, SendLoss, ReceiveLoss\n");
            fclose (file);
        }


        printf ("Client: Data size: %u KB\n", ui32SizeInKB);
        printf ("Client: Message size: %hu Bytes\n", (int) ui16MessageSizeInBytes);
        printf ("Client: number of iterations: %hu\n", (int) ui16Iterations);
        //printf ("Client: will connect to host %s on port %hu\n\n", (const char*) remoteHost, (int) ui16Port);
        printf ("Client: will connect to host %s on port %hu for Mockets, Sockets and SCTP, and on port %hu for UDT\n\n", (const char*) remoteHost, ui16Port, ui16Port + 2);

        Statistics mocketStats, socketStats, sctpSocketStats, udtStats;
        AtomicVar<double> dSocketThroughput(0.0), dMocketThroughput(0.0), dSCTPSocketThroughput(0.0), dUDTThroughput(0.0);
        const double dTotalThroughputPerIterationInKB = ui16ConcurrentFlows * ui32SizeInKB * (1000.0L / 1024.0L);
        for (int i = 0; i < (int) ui16Iterations; i++) {
            static int64 ui64OverallSocketStartTime = 0, ui64OverallSocketEndTime = 0;
            static int64 ui64OverallMocketStartTime = 0, ui64OverallMocketEndTime = 0;
            static int64 ui64OverallsctpSocketStartTime = 0, ui64OverallsctpSocketEndTime = 0;
            static int64 i64OverallUDTStartTime = 0, i64OverallUDTEndTime = 0;
            printf ("Client: Iteration number %d\n", i);

            if (bSockets) {
                vector<ClientTaskThread> threadVector;
                // Create threads
                for (int j = 0; j < ui16ConcurrentFlows; j++) {
                    threadVector.push_back (ClientTaskThread ((i * ui16ConcurrentFlows) + j, remoteHost, ui16Port, "sockets", bEnableKeyExchange,
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
                    printf ("Done with Socket Tests - Sleeping for %d seconds before starting Mocket Test...\n\n", ui16SecToSleep);
                    sleepForMilliseconds (ui16SecToSleep * 1000);
                }
            }
            if (bMockets) {
                // Create threads
                vector<ClientTaskThread> threadVector;
                for (int j = 0; j < ui16ConcurrentFlows; j++) {
                    threadVector.push_back (ClientTaskThread ((i * ui16ConcurrentFlows) + j, remoteHost, ui16Port, "mockets", bEnableKeyExchange,
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
                
                if (bSCTPSockets) {
                    printf ("Done with Mockets Tests - Sleeping for %d seconds before starting SCTPSockets Test...\n\n", ui16SecToSleep);
                    sleepForMilliseconds (ui16SecToSleep * 1000);
                }
            }
         
            if (bSCTPSockets) {
                vector<ClientTaskThread> threadVector;
                // Create threads
                for (int j = 0; j < ui16ConcurrentFlows; j++) {
                    threadVector.push_back (ClientTaskThread ((i * ui16ConcurrentFlows) + j, remoteHost, ui16Port, "sctpSockets", bEnableKeyExchange,
                                                              ui32SizeInKB, ui16MessageSizeInBytes, &sctpSocketStats, dSCTPSocketThroughput));
                }
                
                printf ("Starting SCTPSocket Test...\n");
                ui64OverallsctpSocketStartTime = getTimeInMilliseconds();
                // Start threads
                for (vector<ClientTaskThread>::iterator iter = threadVector.begin(); iter != threadVector.end (); iter++) {
                    (*iter).start(false);
                }
                // Join threads before moving on to the next iteration
                for (vector<ClientTaskThread>::iterator iter = threadVector.begin(); iter != threadVector.end (); iter++) {
                    (*iter).join();
                }
                ui64OverallsctpSocketEndTime = getTimeInMilliseconds();
                printf ("\n");

                if (bUDT) {
                    printf ("Done with SCTP Tests - Sleeping for %d seconds before starting UDT Test...\n\n", ui16SecToSleep);
                    sleepForMilliseconds (ui16SecToSleep * 1000);
                }
            }

            if (bUDT) {
                // Create threads
                std::vector<ClientTaskThread> threadVector;
                for (int j = 0; j < ui16ConcurrentFlows; j++) {
                    threadVector.push_back (ClientTaskThread ((i * ui16ConcurrentFlows) + j, remoteHost, ui16Port + 2, "udt", bEnableKeyExchange,
                                                              ui32SizeInKB, ui16MessageSizeInBytes, &udtStats, dUDTThroughput));
                }

                printf ("Starting UDT Test...\n");
                i64OverallUDTStartTime = getTimeInMilliseconds();
                // Start threads
                for (std::vector<ClientTaskThread>::iterator iter = threadVector.begin(); iter != threadVector.end (); iter++) {
                    (*iter).start(false);
                }
                // Join threads before moving on to the next iteration
                for (std::vector<ClientTaskThread>::iterator iter = threadVector.begin(); iter != threadVector.end (); iter++) {
                    (*iter).join();
                }
                i64OverallUDTEndTime = getTimeInMilliseconds();
                printf ("\n");

                if (bSockets) {
                    printf ("Done with UDT Tests - Sleeping for %d seconds before starting Sockets Test...\n\n", ui16SecToSleep);
                    sleepForMilliseconds (ui16SecToSleep * 1000);
                }
            }

            printf ("-----------------------------------------\n");
            printf ("Total number of protocols: %d\n", (bSockets + bMockets + bSCTPSockets + bUDT) * ui16ConcurrentFlows);

            if (bSockets) {
                if (ui16ConcurrentFlows > 1) {
                    printf ("Socket Iteration Throughput:      %10.4f\n", dTotalThroughputPerIterationInKB /
                                                                        (ui64OverallSocketEndTime - ui64OverallSocketStartTime));
                }
                printf ("Socket Stats:: Average:         %10.4f\n", socketStats.getAverage());
                if (i > 0) {
                    printf ("Socket Stats:: St Deviation:    %10.4f\n", socketStats.getStDev());
                }
            }
            if (bMockets) {
                if (ui16ConcurrentFlows > 1) {
                    printf ("Mocket Iteration Throughput:      %10.4f\n", dTotalThroughputPerIterationInKB /
                                                                        (ui64OverallMocketEndTime - ui64OverallMocketStartTime));
                }
                printf ("Mocket Stats:: Average:         %10.4f\n", mocketStats.getAverage());
                if (i > 0) {
                    printf ("Mocket Stats:: St Deviation:    %10.4f\n", mocketStats.getStDev());
                }
            }
            if (bSCTPSockets) {
                if (ui16ConcurrentFlows > 1) {
                    printf ("SCTPSocket Iteration Throughput:      %10.4f\n", dTotalThroughputPerIterationInKB /
                                                                        (ui64OverallsctpSocketEndTime - ui64OverallsctpSocketStartTime));
                }
                printf ("SCTPSocket Stats:: Average:     %10.4f\n", sctpSocketStats.getAverage());
                if (i > 0) {
                    printf ("SCTPSocket Stats:: St Deviation:%10.4f\n", sctpSocketStats.getStDev());
                }
            }

            if (bUDT) {
                if (ui16ConcurrentFlows > 1) {
                    printf ("UDT Iteration Throughput:    %10.4f\n",
                            dTotalThroughputPerIterationInKB / (i64OverallUDTEndTime - i64OverallUDTStartTime));
                }
                printf ("UDT Stats:: Average:       %10.4f\n", udtStats.getAverage());
                if (i > 0) {
                    printf ("UDT Stats:: St Deviation:  %10.4f\n", udtStats.getStDev());
                }
            }

            printf ("-----------------------------------------\n");
            if (i != (int) (ui16Iterations - 1)) {
                printf ("Sleeping for %d seconds before next iteration...\n", ui16SecToSleep);
                sleepForMilliseconds (ui16SecToSleep * 1000);
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
           if (bSCTPSockets) {
                dSpeedKBytesPerMS = dSCTPSocketThroughput / (ui16Iterations * ui16ConcurrentFlows);
                printf ("Average SCTPSocket Throughput = %.2f KB/s\n", dSpeedKBytesPerMS);
            }
            if (bUDT) {
                dSpeedKBytesPerMS = dUDTThroughput / (ui16Iterations * ui16ConcurrentFlows);
                printf ("Average UDT Throughput = %.2f KB/s\n", dSpeedKBytesPerMS);
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
