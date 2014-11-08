/* 
 * File:   IntDataTestUnrelUnseqServer.cpp
 * Author: ebenvegnu
 *
 * Created on March 2, 2012, 8:31 AM
 */

#include <stdio.h>
#include <stdlib.h>

#include "Logger.h"
#include "MessageSender.h"
#include "Mocket.h"
#include "NLFLib.h"
#include "ServerMocket.h"
#include "Thread.h"
#include "Statistics.h"

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#define MAX_MESSAGE_SIZE 1450
#define DEFAULT_MESSAGE_SIZE_IN_BYTES 1024
#define DEFAULT_TOTAL_SIZE_IN_KB 1024

using namespace std;
using namespace NOMADSUtil;

/*
 * 
 */
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
    int rc;
    static char buf[MAX_MESSAGE_SIZE];
    int64 i64StartTime;

    printf ("ConnHandler::run: client handler thread started for an incoming mockets connection\n");
    _pMocket->setIdentifier ("IntDataTest-Server");
    _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    uint32 ui32Count = 0;
    float fBPS;
    int64 i64CurrTime;
    uint32 ui32ElapsedTime;
    i64StartTime = getTimeInMilliseconds();
    while (true) {
        if ((rc = _pMocket->receive (buf, sizeof (buf))) < 0) {
            printf ("ConnHandler::run: receive returned %d; closing connection\n", rc);
            break;
        }
        //printf ("ConnHandler::run: receive returned %d\n", rc);
        ui32Count += rc;
        i64CurrTime = getTimeInMilliseconds();
        ui32ElapsedTime = (uint32) (i64CurrTime - i64StartTime);
        if (ui32ElapsedTime > 1000) {
            fBPS = (ui32Count / (ui32ElapsedTime / 1000.0)) * 8;
            printf ("Received: %.1f bps\t %.1f Kbps\t %.1f Mbps\t %.lf Bps\n", fBPS, fBPS / 1024, fBPS / (1024*1024), fBPS/8);
                i64StartTime = i64CurrTime;
                ui32Count = 0;
        }
        else {
           // printf ("received a packet of size %d from %s:%d\n", rc, senderAddr.getIPAsString(), (int) senderAddr.getPort(), (int) ui8Buf);
        }       
    }
    
    _pMocket->close();
    delete _pMocket;
    printf ("ConnHandler::run: client handler thread finished\n");
}

double doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, uint16 ui16MessageSizeInBytes)
{
    int rc;
    static double dThroughput;
    static int64 i64StartTime, i64EndTime;
    static unsigned int iTime;
    static unsigned char *buf = new unsigned char[ui16MessageSizeInBytes];
    srand ((unsigned int) getTimeInMilliseconds());

    // Fill the buffer with random data
    for (int i = 0; i < ui16MessageSizeInBytes; i++) {
        buf[i] = (char) rand();
    }
    
    Mocket pm;
    pm.setIdentifier ("IntDataTest-Client");
    pm.registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    if (0 != (rc = pm.connect (pszRemoteHost, usRemotePort))) {
        fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                 pszRemoteHost, usRemotePort, rc);
        printf ("doClientTask: Unable to connect\n");
        return -1;
    }
    printf ("doClientTask: connected\n");

    MessageSender sender = pm.getSender (false, false);
    //MessageSender sender = pm.getSender (true, true);

    int iBytesSent = 0;
    i64StartTime = getTimeInMilliseconds();
    while (true) {
        sender.send (buf, ui16MessageSizeInBytes);
        //iBytesSent += ui16MessageSizeInBytes;
        //printf ("doClientTask: %lu bytes\n", iBytesSent);
    }
}

int main(int argc, char** argv)
{
    bool bServer = false;
    bool bClient = false;
    uint16 ui16Port = 1234;
    uint16 ui16MessageSizeInBytes = DEFAULT_MESSAGE_SIZE_IN_BYTES;
    String remoteHost;
    bool bNoLogging = false;
    
    int i = 1;
    while (i<argc) {
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
        else if ((0 == stricmp (argv[i], "-msgsize")) || (0 == stricmp (argv[i], "-messagesize"))) {
            i++;
            if (i < argc) {
                ui16MessageSizeInBytes = (uint16) atoi (argv[i]);
                if (ui16MessageSizeInBytes > MAX_MESSAGE_SIZE) {
                    ui16MessageSizeInBytes = MAX_MESSAGE_SIZE;
                }
            }
        }
        else if (0 == stricmp (argv[i], "-nolog")) {
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
        fprintf (stderr, "usage: %s {-server | -client -remotehost <remotehost>} [-port <port>] [-msgsize | -messagesize <size_in_bytes>] [-nolog]\n", argv[0]);
        return -1;
    }
    
    if (bServer) {
        if (!bNoLogging) {
            pLogger = new Logger();
            pLogger->initLogFile ("intDataTestUnrelUnseq-server.log", false);
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();
        }
        printf ("Creating a ServerMocket on port %d\n", (int) ui16Port);
        ServerMocketThread *pSMT = new ServerMocketThread (ui16Port);
        pSMT->run();
    }
    else if (bClient) {
        if (!bNoLogging) {
            pLogger = new Logger();
            pLogger->initLogFile ("intDataTestUnrelUnseq-client.log", false);
            pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
            pLogger->enableFileOutput();
            pLogger->disableScreenOutput();
        }
        printf ("Client: will connect to host %s on port %d\n\n", (const char*) remoteHost, (int) ui16Port);
        printf ("Starting Mocket Test...\n");
        int rc = 0;
        if (0 > (rc = doClientTask (remoteHost, ui16Port, ui16MessageSizeInBytes))) {
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
        return true;
    }
    else {
        return false;
    }
}
