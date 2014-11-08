#include "Mocket.h"
#include "MessageSender.h"
#include "ServerMocket.h"

#include "Logger.h"

#include <stdio.h>

#if defined (WIN32)
    #define stricmp _stricmp
#elif defined (UNIX)
    #define stricmp strcasecmp
    #include <strings.h>
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
            pLogger->initLogFile ("gathersendtestserver.log", false);
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
            pLogger->initLogFile ("gathersendtestclient.log", false);
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
    char abuf[2048];
    char bbuf[2048];
    char cbuf[2048];
    int iMCount = 0;
    MessageSender sender = pMocket->getSender (true, true);
    for (int i = 0; i < 2048; i++) {
        abuf[i] = 'a';
        bbuf[i] = 'b';
        cbuf[i] = 'c';
    }
    for (int i = 1; i < 2048; i++) {
        if (sender.gsend (abuf, i, bbuf, 20, cbuf, 2048, NULL)) {
            fprintf (stdout, "server: gsend failed\n");
            return -1;
        }
        printf ("sent message %d\n", ++iMCount);
    }
    for (int i = 1; i < 2048; i++) {
        if (sender.gsend (abuf, 20, bbuf, i, cbuf, 2048, NULL)) {
            fprintf (stdout, "server: gsend failed\n");
            return -2;
        }
        printf ("sent message %d\n", ++iMCount);
    }
    for (int i = 1; i < 2048; i++) {
        if (sender.gsend (abuf, i, bbuf, i, cbuf, 2048, NULL)) {
            fprintf (stdout, "server: gsend failed\n");
            return -3;
        }
        printf ("sent message %d\n", ++iMCount);
    }
    for (int i = 1; i < 2048; i++) {
        if (sender.gsend (abuf, i, bbuf, i, cbuf, i, NULL)) {
            fprintf (stdout, "server: gsend failed\n");
            return -4;
        }
        printf ("sent message %d\n", ++iMCount);
    }
    return 0;
}

int handleConnectionToServer (Mocket *pMocket)
{
    char buf[10240];
    int iMCount = 0;
    while (true) {
        for (int i = 1; i < 2048; i++) {
            int rc = pMocket->receive (buf, sizeof (buf));
            if (rc <= 0) {
                fprintf (stdout, "client: receive() returned %d - terminating\n", rc);
                return -1;
            }
            if (rc != i+20+2048) {
                fprintf (stdout, "client: receive() returned incorrect number of bytes\n");
                return -2;
            }
            for (int j = 0; j < i; j++) {
                if (buf[j] != 'a') {
                    fprintf (stdout, "client: validation 1 failed\n");
                    return -3;
                }
            }
            for (int j = 0; j < 20; j++) {
                if (buf[i+j] != 'b') {
                    fprintf (stdout, "client: validation 2 failed\n");
                    return -4;
                }
            }
            for (int j = 0; j < 2047; j++) {
                if (buf[i+20+j] != 'c') {
                    fprintf (stdout, "client: validation 3 failed\n");
                    return -5;
                }
            }
            printf ("received message %d\n", ++iMCount);
        }
        for (int i = 1; i < 2048; i++) {
            int rc = pMocket->receive (buf, sizeof (buf));
            if (rc <= 0) {
                fprintf (stdout, "client: receive() returned %d - terminating\n", rc);
                return -6;
            }
            if (rc != i+20+2048) {
                fprintf (stdout, "client: receive() returned incorrect number of bytes\n");
                return -7;
            }
            for (int j = 0; j < 20; j++) {
                if (buf[j] != 'a') {
                    fprintf (stdout, "client: validation 4 failed\n");
                    return -8;
                }
            }
            for (int j = 0; j < i; j++) {
                if (buf[20+j] != 'b') {
                    fprintf (stdout, "client: validation 5 failed\n");
                    return -9;
                }
            }
            for (int j = 0; j < 2047; j++) {
                if (buf[20+i+j] != 'c') {
                    fprintf (stdout, "client: validation 6 failed\n");
                    return -10;
                }
            }
            printf ("received message %d\n", ++iMCount);
        }
        for (int i = 1; i < 2048; i++) {
            int rc = pMocket->receive (buf, sizeof (buf));
            if (rc <= 0) {
                fprintf (stdout, "client: receive() returned %d - terminating\n", rc);
                return -11;
            }
            if (rc != i+i+2048) {
                fprintf (stdout, "client: receive() returned incorrect number of bytes\n");
                return -12;
            }
            for (int j = 0; j < i; j++) {
                if (buf[j] != 'a') {
                    fprintf (stdout, "client: validation 7 failed\n");
                    return -13;
                }
            }
            for (int j = 0; j < i; j++) {
                if (buf[i+j] != 'b') {
                    fprintf (stdout, "client: validation 8 failed\n");
                    return -14;
                }
            }
            for (int j = 0; j < 2047; j++) {
                if (buf[i+i+j] != 'c') {
                    fprintf (stdout, "client: validation 9 failed\n");
                    return -15;
                }
            }
            printf ("received message %d\n", ++iMCount);
        }
        for (int i = 1; i < 2048; i++) {
            int rc = pMocket->receive (buf, sizeof (buf));
            if (rc <= 0) {
                fprintf (stdout, "client: receive() returned %d - terminating\n", rc);
                return -16;
            }
            if (rc != i+i+i) {
                fprintf (stdout, "client: receive() returned incorrect number of bytes\n");
                return -17;
            }
            for (int j = 0; j < i; j++) {
                if (buf[j] != 'a') {
                    fprintf (stdout, "client: validation 10 failed\n");
                    return -18;
                }
                if (buf[i+j] != 'b') {
                    fprintf (stdout, "client: validation 11 failed\n");
                    return -19;
                }
                if (buf[i+i+j] != 'c') {
                    fprintf (stdout, "client: validation 12 failed\n");
                    return -20;
                }
            }
            printf ("received message %d\n", ++iMCount);
        }
    }
    return 0;
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    if (ulMilliSec > 10000) {
        return true;
    }
    else {
        return false;
    }
}
