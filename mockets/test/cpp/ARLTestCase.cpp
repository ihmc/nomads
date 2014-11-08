// if debugging on windows, include memory allocation checking
#if defined(WIN32) && defined(_DEBUG)
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#if defined(UNIX)
#define _strdup strdup
#endif

#include <stdio.h>
#include "Mocket.h"
#include "MessageSender.h"
#include "ServerMocket.h"
#include "Logger.h"
#include "Thread.h"
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

using namespace NOMADSUtil;

int rootTime = -1;
int getNowTimeMSec()
    {
    unsigned int deltaSeconds;
#ifdef WIN32
    struct _timeb x;
    _ftime64_s( &x );
#else
    struct timeb x;
    ftime( &x );
#endif
    if( rootTime == -1 )rootTime = (unsigned int)x.time;
    deltaSeconds = ((unsigned int)x.time) - rootTime;
    return (unsigned int)x.millitm + (1000 * deltaSeconds);
    }

int *gblHistoryTime = 0;  //store the time in msec of when message is sent
#define HistoryLen  5000
#define SERVER_PORT 4000

int lagMax = 1000;       //longest lag allowed in msec (data is TOO OLD)
float sfps = 30.0;       //send frames per sec
float rfps = 30.0;       //rcv frames per sec
int globalSendDelay;     //delay in msec to achieve sfps
int globalReceiveDelay;  //delay in msec to achieve rfps
int globalSendId;        //last messageID sent

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

class Handler : public Thread
    {
    public:
        bool killSender, running;
        Handler(Mocket *pm) : running(false), killSender(false), usender(0)
            {
            _pm = pm;
            }
        ~Handler()
            {
            killSender = true;
            while(running)sleepForMilliseconds(100);
            if( usender )delete usender;
            delete _pm;
            }
        void run (void)
            {
            int bigbuf[2500];
            int id = 0;
            usender = new MessageSender( _pm->getSender(false, true ) );
            running = true;
            //  send data to client
            for(;;)
                {
                if( killSender )break;
                bigbuf[0] = id;                             //put id into message
                gblHistoryTime[ id ] = getNowTimeMSec();    //save the time this id was sent
                usender->send( bigbuf, sizeof( bigbuf ) );
                globalSendId = id++;                        //next id
                if( id >= HistoryLen )id = 0;               //modulo length of history DB
                sleepForMilliseconds(globalSendDelay);
                }
            running = false;
            }
    private:
        Mocket *_pm;
        MessageSender *usender;
    };

class ServerListen : public Thread
    {
    public:
        ~ServerListen()
            {
            if( pHandler )delete pHandler;
            printf("Closing ServerMocket\n");
            sm->close();
            // wait for close to complete
            sleepForMilliseconds(500);
            printf("Deleting ServerMocket\n");
            delete sm;
            printf("ServerMocket DELETED\n");
            }
        ServerListen() : pHandler(0)
            {
            sm = new ServerMocket();
            sm->listen (SERVER_PORT);
            }
        void run (void)
            {
            while (true)
                {
                printf("\n...Waiting for a client to connect\n");
                Mocket *pm = sm->accept();
                if (pm)
                    {
                    printf ("\nMocket %p: Server got a new client\n", pm );
                    pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL );
                    if( pHandler )
                        {
                        delete pHandler;
                        pHandler = 0;
                        }
                    pHandler = new Handler(pm);
                    pHandler->start();
                    }
                else break;
                }
            }

    private:
        Handler *pHandler;  // assume only one client
        ServerMocket *sm;
    };

class Connector : public Thread
    {
    public:
        bool killflag, running, enablePrint;;
        Connector( char *serverHost ) :  pm(0), killflag(false), running(false),enablePrint(true)
            {
            _serverHost = _strdup( serverHost );
            }
        ~Connector()
            {
            if( pm != 0 )
                {
                killflag = true;
                while( running )sleepForMilliseconds( 100 );
                printf("Begin deleting connector mocket\n");
                delete pm;   // sometimes crash <========================
                printf("Connector mocket deleted\n");
                }
            }
        void connectToServer()
            {
            int rc;
            if( pm == 0 )
                {
                pm = new Mocket();
                printf ("Mocket %p: Client Mocket created\n", pm);
                }
            pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL );
            printf("Mocket %p: WAITING to connect to %s\n", pm, _serverHost );
            while (0 != (rc = pm->connect (_serverHost, SERVER_PORT)))
                {
                fprintf (stderr, "failed to connect to host %s; rc = %d\n", _serverHost, rc);
                sleepForMilliseconds(5000);
                }
            if( rc == 0 )
                {
                printf("Mocket %p: Connect SUCCESS to %s\n", pm, _serverHost );
                }
            }

        void run()
            {
            int bad = 0, good = 0;
            int rcvTime;
            int pstate = 0;
            int recId;
            int lagFrameCount;
            int bf[2500];
            bf[0] = 444444;
            connectToServer();
            running = true;
            for(;;)
                {
                if( killflag )break;
                int tBegin = getNowTimeMSec(); //get time to start receiving
                int rc = pm->receive(bf, sizeof(bf));
                rcvTime = getNowTimeMSec();    //get time message is received in msec
                if( rc < 0 )break;
                recId = bf[0];
                if( recId < 0 || recId >= HistoryLen )
                    {
                    printf("%d: BAD DATA recID: 0x%x, lastGood: %d    \r", ++bad, recId, good );
                    sleepForMilliseconds(globalReceiveDelay);
                    continue;
                    }
                good = recId;
                lagFrameCount = globalSendId - recId;
                int dt = rcvTime-tBegin;       //how long we were stuck in receive
                int lagMsec = rcvTime-gblHistoryTime[recId];
                if( enablePrint )
                    {
                    if( lagFrameCount > 0 )
                        {
                        if( pstate != 1 )printf("\n");
                        printf("sfps:%f, rfps:%f rcvId:%d sndId:%d, lagFrameCount:%d(%d msec)   \n", sfps, rfps, recId, globalSendId, lagFrameCount, lagMsec  );
                        pstate = 1;
                        }
                    else
                        {
                        if( dt > 2000 )  // detects bursty modes > 2 secs (problem seems to be fixed now)
                            {
                            if( pstate != 2 )printf("\n");
                            printf("sfps:%2f, rfps:%2f len:%d dTime:%d, rcvId:%d      \n", sfps, rfps, rc, dt, recId );
                            pstate = 2;
                            }
                        else
                            {
                            if( pstate != 3 )printf("\n");
                            printf("sfps:%2f, rfps:%2f len:%d dTime:%d, rcvId:%d, lag:%d msec      \r", sfps, rfps, rc, dt, recId, lagMsec );
                            pstate = 3;
                            }
                        }
                    fflush( stdout );
                    }
                // sleep at specified frame rate if not too far behind
                if( lagMsec < lagMax )
                    {
                    int delay = globalReceiveDelay - lagMsec;
                    if( delay > 0 )sleepForMilliseconds(delay);
                    }
                }
            running = false;
            }
    private:
        Mocket *pm;
        char *_serverHost;
    };


int main (int argc, char *argv[])
    {
    char hn[500];
    gblHistoryTime = (int *)malloc( sizeof(int) * HistoryLen );
    globalSendDelay = (int)(1000.0f/sfps);
    globalReceiveDelay = (int)(1000.0f/rfps);
    pLogger = new Logger();
    pLogger->initLogFile ("TestServer1.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);

    Connector *conn;
    ServerListen *listen;
    listen = new ServerListen;
    listen->start();

    gethostname( hn, sizeof(hn) );
    conn = new Connector( hn );
    conn->start();
    for(;;)
        {
        char inp[100];
        //
        //  q - quit
        //  r##### - set receive fps default 5 fps
        //  s###### - set send delay default is 40 fps
        //  t###### - set tooOld in msec  (maximum lag time tolerated before tossing packets)
        //  p - toggle print on off
        //
        fgets( inp, sizeof(inp), stdin );
        if( inp[0] == 'q' )break;
        else if( inp[0] == 'p' )
            {
            conn->enablePrint = !conn->enablePrint;
            }
        else if( inp[0] == 'r' )
            {
            sscanf( inp+1, "%f", &rfps );
            if( rfps <= 0 )rfps = 5.0f;
            globalReceiveDelay = (int)(1000.0f/rfps);
            }
        else if( inp[0] == 's' )
            {
            sscanf( inp+1, "%f", &sfps );
            if( sfps <= 0 )sfps = 40.0f;
            globalSendDelay = (int)(1000.0f/sfps);
            }
        else if( inp[0] == 't' )
            {
            int ival;
            sscanf( inp+1, "%d", &ival );
            if( ival < 1 )ival = 1;
            lagMax = ival;
            }
        }
    delete conn;
    delete listen;
    printf("\nHit <cr> to quit\n");
    getchar();
    delete pLogger;
    free(gblHistoryTime);

#if defined(WIN32) && defined(_DEBUG)
    _CrtDumpMemoryLeaks();
#endif
    return 0;
    }

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
    {
    printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 10000)
        {
        return true;
        }
    else
        {
        return false;
        }
    }
