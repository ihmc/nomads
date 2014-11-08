#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <csignal>

#include "InetAddr.h"
#include "Logger.h"
#include "Mocket.h"
#include "MessageSender.h"
#include "NLFLib.h"
#include "ServerMocket.h"
#include "Socket.h"
#include "TCPSocket.h"
#include "Thread.h"

#if defined (LINUX) || defined (UNIX)
    #define stricmp strcasecmp
#endif
#if defined (_DEBUG)
    #include <crtdbg.h>
#endif

using namespace NOMADSUtil;        
        
volatile bool continue_flag = true;

const int PACKET_PER_SEC = 50;
const char *CLIENT_STATS_FILENAME = "BioEnvMonitoringStation-Client-Stats.txt";
const char *SERVER_STATS_FILENAME = "BioEnvMonitoringStation-Server-Stats.txt";
const char *CLIENT_LOG_FILENAME   = "BioEnvMonitoringStation-Client.log";
const char *SERVER_LOG_FILENAME   = "BioEnvMonitoringStation-Server.log";

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

void sigint_handler (int signo)
{
    (void)signo; // just to stop the compiler from whining
    continue_flag = false;
}

class MessageServerMocketThread : public Thread
{
    public:
        MessageServerMocketThread (int usServerPort);
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

MessageServerMocketThread::MessageServerMocketThread (int usServerPort)
{
    _pServerMocket = new ServerMocket();
    _pServerMocket->listen (usServerPort);
}

void MessageServerMocketThread::run (void)
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
    char buf[1024];

    puts ("ConnHandler: client handler thread started");
    if (_useMockets) {
        _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

        // Save results to a file
        FILE *file = fopen (SERVER_STATS_FILENAME, "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file %s\n", SERVER_STATS_FILENAME);
            return;
        }
        
        while (continue_flag) {
            int rc;
            if ((rc = _pMocket->receive (buf, sizeof (buf))) > 0) {
                fprintf (file, "[%lu] #%u\n", (unsigned long) getTimeInMilliseconds(), *((uint32*)buf));
            } else {
                printf ("receive returned error code %d: closing connection.\n", rc);
                break;
            }
        }

        fclose (file);
        _pMocket->close();
        delete _pMocket;
    } else {
        // Save results to a file
        FILE *file = fopen (SERVER_STATS_FILENAME, "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file %s\n", SERVER_STATS_FILENAME);
            return;
        }
        
        while (continue_flag) {
            int rc;
            if ((rc = _pSocket->receive (buf, sizeof (buf))) > 0) {
                fprintf (file, "[%lu] #%u\n", (unsigned long) getTimeInMilliseconds(), *((uint32*)buf));
            } else {
                printf ("receive returned error code %d: closing connection.\n", rc);
                break;
            }
        }

        fclose (file);
        _pSocket->disconnect();
        delete _pSocket;
    }
    puts ("ConnHandler: client handler thread finished");
}

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, bool bUseMockets)
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
        Mocket *pm = new Mocket();
        pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

        puts ("doClientTask: Using Mockets:Before connect");
        if (0 != (rc = pm->connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            puts ("doClientTask: Unable to connect");
            delete pm;
            return -11;
        }
        
        MessageSender rlsq = pm->getSender (true, true);
        MessageSender ursq = pm->getSender (false, true);

        // open stats file
        FILE *file = fopen (CLIENT_STATS_FILENAME, "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file %s\n", CLIENT_STATS_FILENAME);
            return -12;
        }

        // mockets client code
        for (int i = 0; continue_flag; ++i) {
            // write sequence number in the first 4 bytes
            *((uint32*)buf) = EndianHelper::htonl ((uint32)i); 
            if (0 == (i % PACKET_PER_SEC)) {
                // send one reliable sequenced packet per second
                rlsq.send (buf, sizeof (buf));
            } else {
                // send an unreliable sequenced packet
                ursq.send (buf, sizeof (buf));
            }            
            sleepForMilliseconds (20);
        }

        fclose (file);
        pm->close();
        delete pm;
    } else {
        TCPSocket socket;
        puts ("doClientTask: Using Sockets:Before connect");
        if (0 != (rc = socket.connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using sockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            puts ("doClientTask: Unable to connect");
            return -11;
        }

        // open stats file
        FILE *file = fopen (CLIENT_STATS_FILENAME, "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file %s\n", CLIENT_STATS_FILENAME);
            return -12;
        }

        // sockets client code
        for (int i = 0; continue_flag; ++i) {
            // write sequence number in the first 4 bytes
            *((uint32*)buf) = EndianHelper::htonl ((uint32)i); 
            socket.send (buf, sizeof (buf));
            sleepForMilliseconds (20);
        }

        fclose (file);
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

    pLogger = new Logger();

    bool bServer = false;
    bool bMockets = false;
    if (argc < 4 || argc > 5 ||
        (stricmp(argv[1], "client") && (bServer = true) && stricmp(argv[1], "server")) || 
        (stricmp(argv[2], "sockets") && (bMockets = true) && stricmp(argv[2], "mockets")) ||
        (argc == 4 && !bServer)) {
        fprintf (stderr, "usage: %s <server|client> <mockets|sockets> <port> [<remotehost>]\n", argv[0]);
        return -1;
    }

    signal (SIGINT, sigint_handler);
    
    if (bServer) {
        // server side code                
        pLogger->initLogFile (SERVER_LOG_FILENAME);
        pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
        pLogger->enableFileOutput();
        pLogger->disableScreenOutput();

        unsigned short usLocalPort = atoi (argv[3]);
        
        if (bMockets) {
            printf ("Creating a ServerMocket on port %d\n", (int) usLocalPort);
            MessageServerMocketThread *pSMT = new MessageServerMocketThread (usLocalPort);
            // call run() instead of start() so that the main thread does not end
            // mauro: that's just ridiculous! we should implement Thread::join instead
            pSMT->run();
        } else {
            printf ("Creating a ServerSocket on port %d\n", (int) usLocalPort);
            ServerSocketThread *pSST = new ServerSocketThread (usLocalPort);
            // call run() instead of start() so that the main thread does not end
            // mauro: that's just ridiculous! we should implement Thread::join instead
            pSST->run(); 
        }
    } else {
        // client side code
        pLogger->initLogFile (CLIENT_LOG_FILENAME);
        pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
        pLogger->enableFileOutput();
        pLogger->disableScreenOutput();

        unsigned short usRemotePort = atoi (argv[3]);
        const char *pszRemoteHost = argv[4];
        
        int rc;
        if (bMockets) {
            printf ("Client Creating a Mocket on port %d\n", (int) usRemotePort);
            if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, true))) {
                fprintf (stderr, "main: doClientTask failed for Mockets with rc = %d\n", rc);
                return -2;
            }
        } else {
            printf ("Client Creating a Socket on port %d\n", (int) usRemotePort);
            if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, false))) {
                fprintf (stderr, "main: doClientTask failed for Sockets with rc = %d\n", rc);
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
    if (ulMilliSec > 5000) {
        // close connection if peer is unreachable for more than 5 seconds
        return true;
    }
    else {
        return false;
    }
}

/*
 * vim: et ts=4 sw=4
 */

