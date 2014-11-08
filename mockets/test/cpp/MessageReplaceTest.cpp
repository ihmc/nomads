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
        
#define MSG_FREQUENCY  1000

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);


class NetworkControllerThread : public Thread
{
	public:
		NetworkControllerThread();
		void run(void);
		bool _run;
	private:
		int _waitTime;
};

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

int connectNetwork()
{
	printf("connecting the network...\n");
	int ret;
	#ifdef WIN32
		ret = system ("connectNetwork.bat");
	#else
		ret = system ("./connectNetwork.sh");
	#endif
	return ret;
}

int disconnectNetwork()
{
	printf("disconnecting the network...\n");

	int ret;
	#ifdef WIN32
		ret = system ("disconnectNetwork.bat");
	#else
		ret = system ("./disconnectNetwork.sh");
	#endif
	return ret;
}

NetworkControllerThread::NetworkControllerThread()
{
}

void NetworkControllerThread::run()
{
	printf("NetworkControllerThread started.\n");

	connectNetwork();
	_run = true;

	while (_run) {
		sleepForMilliseconds(10000);
		if (_run) {
			disconnectNetwork();
		}
		if (_run) {
			sleepForMilliseconds(10000);
		}
		connectNetwork();
	}

	connectNetwork();
}

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
	int msgSize = 1024;
    char buf[1024];

    int iBytesRead = 0;
    int iBytesToRead = 0;
    int iTime = 0;
	bool cont = true;

    printf ("ConnectionHandler: client handler thread started\n");
	int32 i32msgId;

	int64 i64msgTS, now, msg0TS, local0TS, catchUpAux;
	int delay, catchUp;

	iBytesToRead = msgSize;

	if (_useMockets) {
        _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

        FILE *file = fopen ("stats-server-msgreplace-mockets-cpp.txt", "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file stats-MsgMockets-cpp.txt\n");
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
			i32msgId = ((int32*)buf)[0];
			i64msgTS = ((int64*)buf)[4];
			now = getTimeInMilliseconds();

			if (i32msgId != 0) {
				delay = (int)(now - ((i64msgTS - msg0TS) + local0TS));
				//delay = i64msgTS - lastRecvTimestamp;
			}
			else {
				msg0TS = i64msgTS;
				local0TS = now;

				delay = 0;
			}
			//lastRecvTimestamp = i64msgTS;

			if (delay < 0) {
				delay = 0;
			}
			if ( (delay > 45) && (catchUpAux == 0) ) {
				// connection was just lost.
				catchUpAux = now;
				catchUp = 0;
			}
			else if ( (delay < 45) && (catchUpAux != 0) ) {
				catchUp = (int)(now - catchUpAux);
				catchUpAux = 0;
			}
			else {
				catchUp = 0;
			}

			printf("Mocket Msg Recvd:: [%lu]\t%lu\t%d\t%d\t%d\n", 
				(long)now, (long)i64msgTS, i32msgId, delay, catchUp);
			fprintf(file, "Mocket Msg Recvd:: [%lu]\t%lu\t%d\t%d\t%d\n", 
				(long)now, (long)i64msgTS, i32msgId, delay, catchUp);
			fflush(file);

			iBytesRead = 0;
		}

		printf("closing the mocket connection\n");
        fclose (file);

		_pMocket->close();

        delete _pMocket;
    }
    else {
		//sockets.

        FILE *file = fopen ("stats-server-msgreplace-sockets-cpp.txt", "a");
        if (file == NULL) {
            fprintf (stderr, "failed to append to file stats-MsgMockets-cpp.txt\n");
            return;
        }

		while (cont)  {
			i = 0;
			while (iBytesRead < iBytesToRead) {
				if ((i = _pSocket->receiveBytes(buf, sizeof(buf))) > 0) {
					iBytesRead += i;
				}
				else {
					printf ("receive returned %d; closing connection\n", i);
					cont = false;
					break;
				}
			}
			i32msgId = ((int32*)buf)[0];
			i64msgTS = ((int64*)buf)[4];
			now = getTimeInMilliseconds();

			if (i32msgId != 0) {
				delay = (int)(now - ((i64msgTS - msg0TS) + local0TS));
			}
			else {
				msg0TS = i64msgTS;
				local0TS = now;
				catchUpAux = 0;
				delay = 0;
			}
			//lastRecvTimestamp = i64msgTS;

			if (delay < 0) {
				delay = 0;
			}
			if ( (delay > 45) && (catchUpAux == 0) ) {
				// connection was just lost.
				catchUpAux = now;
				catchUp = 0;
			}
			else if ( (delay < 45) && (catchUpAux != 0) ) {
				catchUp = (int)(now - catchUpAux);
				catchUpAux = 0;
			}
			else {
				catchUp = 0;
			}

			printf("Socket Msg Recvd:: [%lu]\t%lu\t%d\t%d\t%d\n", 
				(long)now, (long)i64msgTS, i32msgId, delay, catchUp);
			fprintf(file, "Socket Msg Recvd:: [%lu]\t%lu\t%d\t%d\t%d\n", 
				(long)now, (long)i64msgTS, i32msgId, delay, catchUp);
			fflush(file);

			iBytesRead = 0;
		}

		_pSocket->disconnect();
		fclose (file);
        
        delete _pSocket;
    }
    printf ("ConnectionHandler: client handler thread finished\n");
}

int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, bool bUseMockets, Statistics *pStats)
{
    int rc;
    static char buf [1024];
	uint32 ui32;

	NetworkControllerThread *nct = new NetworkControllerThread();
	connectNetwork();

	int numIterations = 1000;
    
	if (bUseMockets) {
		printf ("Connected to server - start sending messages\n");

        Mocket *pMocket = new Mocket();
        pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

        printf ("doClientTask: Using Mockets:Before connect\n");
        if (0 != (rc = pMocket->connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            printf ("doClientTask: Unable to connect\n");
            delete pMocket;
            return -1;
        }

		nct->start();

		MessageSender sender = pMocket->getSender (true, true);
		for (ui32 = 0; ui32 < numIterations; ui32++) {
			printf("%d\n", ui32);
			((int*)buf)[0] = ui32;
			((int64*)buf)[4] = getTimeInMilliseconds();
			sender.replace (buf, sizeof (buf), 1, 1);
			sleepForMilliseconds(MSG_FREQUENCY);
		}

		printf("closing the mocket\n");
		pMocket->close();

		return 0;
    }
    else {
        TCPSocket socket;
        if (0 != (rc = socket.connect (pszRemoteHost, usRemotePort))) {
            fprintf (stderr, "doClientTask: failed to connect using sockets to remote host %s on port %d; rc = %d\n",
                     pszRemoteHost, usRemotePort, rc);
            return -3;
        }
		
		nct->start();

		for (ui32 = 0; ui32 < numIterations; ui32++) {
			printf("%d\n", ui32);
			((int*)buf)[0] = ui32;
			((int64*)buf)[4] = getTimeInMilliseconds();
			socket.send (buf, sizeof (buf));
			sleepForMilliseconds(MSG_FREQUENCY);
		}

		printf("closing the socket\n");
        socket.disconnect();
    }
    return 0;
}

int main (int argc, char *argv[])
{
//    pLogger = new Logger();

    #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (58);
    #endif

    if (((argc != 3) && (argc != 4)) && ((argc != 3) && (argc != 5))) {
        fprintf (stderr, "usage: %s <server|client> <port> <sockets|mockets> [<remotehost>]\n", argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            // On a server - start up both Mocket and socket
/*
			pLogger->initLogFile ("intDataTest-server.log");
			pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
			pLogger->enableFileOutput();
			pLogger->disableScreenOutput();
*/
            unsigned short usPort = atoi (argv[2]);
            
            printf ("Creating a ServerMocket and ServerSocket on port %d\n", (int) usPort);
            ServerMocketThread *pSMT = new ServerMocketThread (usPort);
            pSMT->start();
            ServerSocketThread *pSST = new ServerSocketThread (usPort);
	        // Call run() instead of start() so that the main thread does not end
            pSST->run(); 
        }
        else if (0 == stricmp (argv[1], "client")) {
/*
			pLogger->initLogFile ("intDataTest-client.log");
			pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
			pLogger->enableFileOutput();
			pLogger->disableScreenOutput();
*/
			unsigned short usRemotePort = atoi (argv[2]);
            /*
			unsigned short usIterations;
            if (argc == 5) {
                usIterations = atoi (argv[4]);

            }
            else {
                usIterations = 1000;
            }
            */
            const char *pszRemoteHost = argv[4];
//            printf ("-->>ClientIterations Number: %d\n", (int) usIterations);
            printf ("Client Creating a Mocket and Socket on port %d\n", (int) usRemotePort);

            Statistics mocketStats, socketStats;
//            for (int i = 0; i < usIterations; i++) {
//                printf ("-->>ClientIterations i Number: %d\n", i);
                int rc;

			if (0 == stricmp (argv[3], "sockets")) {
				if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, false, &socketStats))) {
                    fprintf (stderr, "main: doClientTask failed for sockets with rc = %d\n", rc);
                    return -2;
                }
			}
			else if (0 == stricmp (argv[3], "mockets")) {
                if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, true, &mocketStats))) {
                    fprintf (stderr, "main: doClientTask failed for mockets with rc = %d\n", rc);
                    return -3;
                }
		    }

//				sleepForMilliseconds (10000);
//            }
        }
    }

    #if defined (WIN32) && defined (_DEBUG)
        _CrtDumpMemoryLeaks();
    #endif

    return 0;
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
//    printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 10000) {
        return true;
    }
    else {
        return false;
    }
}
