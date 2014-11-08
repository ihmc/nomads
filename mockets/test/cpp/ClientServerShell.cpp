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

#include <stdio.h>
#include <string.h>

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
int doClientTask (Mocket *pMocket);
int doServerTask (Mocket *pMocket);

int doClientTask (Mocket *pMocket)
{
    printf ("This is the client - hit return to finish\n");
    getchar();
    return 0;
}

int doServerTask (Mocket *pMocket)
{
    printf ("This is the client handler on the server - hit return to finish\n");
    getchar();
    return 0;
}

class ConnHandler : public Thread
{
    public:
        ConnHandler (Mocket *pMocket);
        void run (void);

    private:
        Mocket *_pMocket;
};

ConnHandler::ConnHandler (Mocket *pMocket)
{
    _pMocket = pMocket;
}

void ConnHandler::run (void)
{
    printf ("ConnHandler: client handler thread started.\n");
    _pMocket->setIdentifier ("ClientServerShell-Server");

    // Do what needs to be done
    doServerTask (_pMocket);

    _pMocket->close();
    delete _pMocket;
    printf ("ConnHandler: client handler thread finished\n");
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

    if (argc < 3) {
        fprintf (stderr, "usage: %s { \"server\" <port> | \"client\" <port> <remotehost> }\n", argv[0]);
        return -1;
    }
    else {
        if (0 == stricmp (argv[1], "server")) {
            pLogger->initLogFile ("ClientServerShell-server.log");
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();

            unsigned short usPort = atoi (argv[2]);

            ServerMocket msm;
            msm.listen (usPort);
            while (true) {
                Mocket *pMocket = msm.accept();
                if (pMocket) {
                    printf ("ServerMocket: got a connection\n");
                    ConnHandler *pHandler = new ConnHandler (pMocket);    /*!!*/ // Memory Leak!
                    pHandler->start();
                }
            }
        }
        else if (0 == stricmp (argv[1], "client")) {
            if (argc < 4) {
                fprintf (stderr, "usage: %s { \"server\" <port> | \"client\" <port> <server ip> }\n", argv[0]);
                return -2;
            }
            pLogger->initLogFile ("ClientServerShell-client.log");
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();

            unsigned short usRemotePort = atoi (argv[2]);
            const char *pszServerIP = argv[3];

            Mocket mm;
            mm.connect (pszServerIP, usRemotePort);

            doClientTask (&mm);

            mm.close();
        }
        else {
            fprintf (stderr, "usage: %s { \"server\" <port> | \"client\" <port> <server ip> }\n", argv[0]);
            return -3;
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
        return true;
    }
    else {
        return false;
    }
}
