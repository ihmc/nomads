#include "Mocket.h"
#include "MessageSender.h"
#include "ServerMocket.h"

#include "Logger.h"

#include <stdio.h>

#if defined (UNIX)
    #define stricmp strcasecmp
    #include <strings.h>
#elif defined (WIN32)
    #define stricmp _stricmp
#endif      

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
int handleConnectionFromClient (Mocket *pMocket);
int handleConnectionToServer (Mocket *pMocket);

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    if (argc < 2) {
        fprintf (stderr, "usage: %s <server|client> <port> [<remotehost>]\n", argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            pLogger->initLogFile ("blockedwriterserver.log", false);
            pLogger->enableFileOutput();
            unsigned short usPort = atoi (argv[2]);
            ServerMocket serverMocket;
            serverMocket.listen (usPort);
            Mocket *pMocket = serverMocket.accept();
            if (pMocket == NULL) {
                fprintf (stderr, "%s: failed to accept a connection\n", argv[0]);
                return -2;
            }
            fprintf (stdout, "server: received a connection\n");
            pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            pMocket->setConnectionLingerTime (15000);
            handleConnectionFromClient (pMocket);
            fprintf (stdout, "server: done with connection to client\n");
            delete pMocket;
            serverMocket.close();
        }
        else if (0 == stricmp (argv[1], "client")) {
            pLogger->initLogFile ("blockedwriterclient.log", false);
            pLogger->enableFileOutput();
            if (argc < 3) {
                fprintf (stderr, "usage: %s client <port> <remotehost>\n", argv[0]);
                return -3;
            }
            unsigned short usRemotePort = atoi (argv[2]);
            const char *pszRemoteHost = argv[3];
            Mocket mocket;
            int rc;
            if (0 != (rc = mocket.connect (pszRemoteHost, usRemotePort))) {
                fprintf (stderr, "%s: failed to connect to %s:%d; rc = %d\n", argv[0], pszRemoteHost, (int)usRemotePort, rc);
                return -4;
            }
            fprintf (stdout, "client: connected\n");
            mocket.registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            handleConnectionToServer (&mocket);
            fprintf (stdout, "client: done with connection to server\n");
            mocket.close();
        }
    }
    return 0;
}

int handleConnectionFromClient (Mocket *pMocket)
{
    char buf[128];
    MessageSender sender = pMocket->getSender (true, true);
    while (true) {
        if (sender.send (buf, sizeof (buf)) < 0) {
            return -1;
        }
    }
    return 0;
}

int handleConnectionToServer (Mocket *pMocket)
{
    char buf[128];
    int64 i64TimeLastPacketReceived = 0;
    while (true) {
        int rc = pMocket->receive (buf, sizeof (buf));
        if (rc != sizeof (buf)) {
            fprintf (stdout, "receive() returned %d; terminating\n", rc);
            return 0;
        }
        fprintf (stdout, "received message\n");
    }
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 10000) {
        return true;
    }
    else {
        return false;
    }
}
