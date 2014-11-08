#include <stdio.h>

#include "Logger.h"
#include "Mocket.h"
#include "MessageSender.h"
#include "ServerMocket.h"
#include "Thread.h"

#if defined (UNIX)
    #include <pthread.h>
#elif defined (WIN32)
    #include <process.h>
#endif

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif

using namespace NOMADSUtil;        
        
bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

class Client : public Thread
{
    public:
        Client (uint16 ui16ClientId, uint16 ui16ServerPort);
        void run (void);

    private:
        uint16 _ui16ClientId;
        uint16 _ui16ServerPort;
};

class Server : public Thread
{
    public:
        Server (uint16 ui16ServerPort);
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

Client::Client (uint16 ui16ClientId, uint16 ui16ServerPort)
{
    _ui16ClientId = ui16ClientId;
    _ui16ServerPort = ui16ServerPort;
}

void Client::run (void)
{
    Mocket mocket;
    if (mocket.connect ("127.0.0.1", _ui16ServerPort)) {
        printf ("Client::run: failed to connect to server on port %d\n", (int) _ui16ServerPort);
        return;
    }
    mocket.registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    char buf[256];
    int iMsgCount = 0;
    while (true) {
        int rc = mocket.receive (buf, sizeof (buf));
        if (rc <= 0) {
            printf ("Client::run: client %d terminating after receiving %d messages - receive returned %d\n", (int) _ui16ClientId, iMsgCount, rc);
            break;
        }
        printf ("Cleint::run: client %d received <%s>\n", (int) _ui16ClientId, buf);
        iMsgCount++;
    }
}

Server::Server (uint16 ui16ServerPort)
{
    _pServerMocket = new ServerMocket();
    _pServerMocket->listen (ui16ServerPort);
}

void Server::run (void)
{
    while (true) {
        Mocket *pMocket = _pServerMocket->accept();
        if (pMocket) {
            printf ("Server: got a connection\n");
            ConnHandler *pHandler = new ConnHandler (pMocket);
            pHandler->start();
        }
        else {
            printf ("Server: terminating\n");
            break;
        }
    }
}

ConnHandler::ConnHandler (Mocket *pMocket)
{
    _pMocket = pMocket;
}

void ConnHandler::run (void)
{
    printf ("ConnectionHandler: client handler thread started\n");
    int64 i64StartTime = getTimeInMilliseconds();
    _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    MessageSender sender = _pMocket->getSender (true, true);
    char msg[256];
    strcpy (msg, "This is the first message - To be or not to be, that is the question");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the second message - I am but mad north by north-west");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the third message - There is something rotten in the state of Denmark");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the fourth message - If you prick us, do we not bleed?");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the fifth message - If you wrong us, shall we not revenge?");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the sixth message - Aye, there's the rub");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the seventh message - A Horse, a horse, my kingdom for a horse");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the eight message - To sleep, perchance to dream");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the ninth message - Frailty, thy name is woman");
    sender.send (msg, strlen(msg)+1);
    strcpy (msg, "This is the tenth message - There is method to my madness");
    sender.send (msg, strlen(msg)+1);
    _pMocket->close();
    printf ("ConnectionHandler: client handler thread finished\n");
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
    pLogger->initLogFile ("OneProcessTest.log");
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    pLogger->disableFileOutput();
    pLogger->disableScreenOutput();
    Server *pServer = new Server (1234);
    pServer->start();
    Client* apClients[20];
    for (uint16 ui16 = 0; ui16 < 20; ui16++) {
        apClients[ui16] = new Client (ui16, 1234);
    }
    for (uint16 ui16 = 0; ui16 < 20; ui16++) {
        apClients[ui16]->start();
    }
    printf ("main: sleeping for 30 seconds\n");
    sleepForMilliseconds (30000);
    printf ("main: terminating\n");
    delete pServer;
    for (uint16 ui16 = 0; ui16 < 20; ui16++) {
        delete apClients[ui16];
    }
#if defined(UNIX)
    pthread_exit (NULL);
#else
    _endthread();    // Causes this thread to terminate without exiting the program
                     // If the program terminates, then all other threads also died
#endif

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
