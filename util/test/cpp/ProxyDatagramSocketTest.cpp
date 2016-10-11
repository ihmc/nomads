#include "ProxyDatagramSocket.h"

#include "InetAddr.h"
#include "Logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (WIN32)
    #define stricmp _stricmp
#elif defined (UNIX)
    #define stricmp strcasecmp
#endif

using namespace NOMADSUtil;

#define MESSAGE_SIZE 180

int doSend (ProxyDatagramSocket *pDGSocket, const char *pszDestAddr, uint16 ui16DestPort);
int doReceive (ProxyDatagramSocket *pDGSocket);

#define checkAndLogMsg if (pLogger) pLogger->logMsg

int main (int argc, char *argv[])
{
    int rc;
    if (((argc != 5) && (argc != 6)) ||
        ((0 != stricmp (argv[3], "send")) && (0 != stricmp (argv[3], "receive")))) {
        fprintf (stderr, "usage: %s <ProxyServerAddr> <ProxyServerPort> send <destIP> <destPort>\n"
                         "       %s <ProxyServerAddr> <ProxyServerPort> receive <bindPort>\n",
                 argv[0], argv[0]);
        return -1;
    }

    bool bSender = false;
    uint16 ui16LocalPort = 0;
    if (0 == stricmp (argv[3], "send")) {
        if (argc != 6) {
            fprintf (stderr, "usage: %s <ProxyServerAddr> <ProxyServerPort> send <destIP> <destPort>\n",
                     argv[0]);
            return -2;
        }
        else {
            bSender = true;
        }
    }
    else if (argc != 5) {
        fprintf (stderr, "usage: %s <ProxyServerAddr> <ProxyServerPort> receive <bindPort>\n",
                 argv[0]);
        return -3;
    }
    else {
        ui16LocalPort = (uint16) atoi (argv[4]);
    }

    Logger logger;
    pLogger = &logger;
    pLogger->initLogFile ("ProxyDatagramSocketTest.log");
    pLogger->enableFileOutput();
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);

    ProxyDatagramSocket pdgs;
    if (0 != (rc = pdgs.init (argv[1], atoi (argv[2]), ui16LocalPort))) {
        checkAndLogMsg ("ProxyDatagramSocketTest::main", Logger::L_SevereError,
                        "failed to initialize ProxyDatagramSocket to connect to server at %s:%d and use port %d\n",
                        argv[1], atoi (argv[2]), ui16LocalPort);
        return -4;
    }

    checkAndLogMsg ("ProxyDatagramSocketTest::main", Logger::L_LowDetailDebug,
                    "Local address is %s (%x); local port is %lu\n",
                    pdgs.getLocalAddr().getIPAsString(), pdgs.getLocalAddr().getIPAddress(), (unsigned long) pdgs.getLocalPort());

    if (0 == stricmp (argv[3], "send")) {
        doSend (&pdgs, argv[4], (uint16) atoi (argv[5]));
    }
    else if (0 == stricmp (argv[3], "receive")) {
        doReceive (&pdgs);
    }
    else {
        fprintf (stderr, "unknown command <%s>; command must be send or receive\n",
                 argv[3]);
        return -5;
    }

    return 0;
}

int doSend (ProxyDatagramSocket *pDGSocket, const char *pszDestAddr, uint16 ui16DestPort)
{
    int rc;
    uint8 ui8MsgBuf [MESSAGE_SIZE];
    uint32 ui32MsgCount = 1;
    InetAddr destAddr (pszDestAddr);

    checkAndLogMsg ("ProxyDatagramSocketTest::main", Logger::L_LowDetailDebug,
                    "sending to address %s (%x) and port %lu\n",
                    destAddr.getIPAsString(), destAddr.getIPAddress(), (unsigned long) ui16DestPort);

    while (true) {
        memcpy (ui8MsgBuf, &ui32MsgCount, 4);
        uint8 ui8FillChar = (uint8) ui32MsgCount;
        for (int i = 4; i < MESSAGE_SIZE; i++) {
            ui8MsgBuf [i] = ui8FillChar++;
        }
        if (0 != (rc = pDGSocket->sendTo (destAddr.getIPAddress(), ui16DestPort, ui8MsgBuf, sizeof (ui8MsgBuf)))) {
            printf ("doSend(): sendTo() failed with rc = %d\n", rc);
            return -1;
        }
        printf ("doSend(): sent message %lu\n", ui32MsgCount);
        ui32MsgCount++;
        sleepForMilliseconds (1000);
    }
    return 0;
}

int doReceive (ProxyDatagramSocket *pDGSocket)
{
    int rc;
    uint8 ui8MsgBuf [MESSAGE_SIZE];
    uint32 ui32MsgId;
    uint32 ui32LastMsgId = 0;
    uint32 ui32TotalLostPackets = 0;
    InetAddr sourceAddr;

    while ((rc = pDGSocket->receive (ui8MsgBuf, sizeof (ui8MsgBuf), &sourceAddr)) > 0) {
        // Validate packet
        memcpy (&ui32MsgId, ui8MsgBuf, 4);
        uint8 ui8ExpectedFillChar = (uint8) ui32MsgId;
        for (int i = 4; i < MESSAGE_SIZE; i++) {
            if (ui8MsgBuf [i] != ui8ExpectedFillChar++) {
                printf ("doReceive(): packet validation failed\n");
                return -1;
            }
        }

        //Find how many packets were lost
        if (ui32MsgId != (ui32LastMsgId + 1)) {
            uint32 ui32LostPackets = (ui32MsgId - (ui32LastMsgId + 1));
            ui32TotalLostPackets += ui32LostPackets;
            printf ("doReceive(): received packet %lu - lost %lu packets (%lu packets in total)\n", ui32MsgId, ui32LostPackets, ui32TotalLostPackets);
        }
        else {
            printf ("doReceive(): received valid packet %lu from <%s:%d>\n", ui32MsgId, sourceAddr.getIPAsString(), (int) sourceAddr.getPort());
        }
        ui32LastMsgId = ui32MsgId;
    }

    return 0;
}
