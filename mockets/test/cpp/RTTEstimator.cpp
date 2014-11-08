#include "UDPDatagramSocket.h"
#include "FTypes.h"
#include "InetAddr.h"
#include "MovingAverage.h"
#include "NLFLib.h"
#include "OSThread.h"

#include <stdio.h>
#include <stdlib.h>

#if defined (UNIX)
    #define stricmp strcasecmp
    #include <strings.h>
#endif


using namespace NOMADSUtil;        
        
#define PORT_NUM 2002
#define MINIMUM_PACKET_SIZE 8
#define MAXIMUM_PACKET_SIZE 65535

void printUsage (void);
int  server (void);
int  client (const char *pszServerIP, uint32 ui32InterPacketInterval, uint16 ui16PacketSize);
void clientReceiver (void *pArg);

UDPDatagramSocket dgSocket;
OSThread clientReceiverThread;
uint32 ui32PacketsSent;

int main (int argc, char *argv[])
{
    if (argc < 2) {
        printUsage();
        return -1;
    }
    if (0 == stricmp (argv[1], "server")) {
        return server();
    }
    else if (0 == stricmp (argv[1], "client")) {
        if (argc < 4) {
            printUsage();
            return -2;
        }
        const char *pszServerIP = argv[2];
        uint32 ui32InterPacketInterval = (uint32) atoi (argv[3]);
        uint16 ui16PacketSize = MINIMUM_PACKET_SIZE;
        if (argc > 4) {
            int iPacketSize = atoi (argv[4]);
            if ((iPacketSize < MINIMUM_PACKET_SIZE) || (iPacketSize > MAXIMUM_PACKET_SIZE)) {
                printUsage();
                return -3;
            }
            else {
                ui16PacketSize = (uint16) iPacketSize;
            }
        }
        ui32PacketsSent = 0;
        return client (pszServerIP, ui32InterPacketInterval, ui16PacketSize);
    }
    else {
        printUsage();
        return -4;
    }
}

void printUsage (void)
{
    fprintf (stderr, "usage: RTTEstimator server\n"
                     "       RTTEstimator client <serverip> <inter-packet interval in ms> [<packetsize> from %d to %d]\n", MINIMUM_PACKET_SIZE, MAXIMUM_PACKET_SIZE);
}

int server (void)
{
    int rc;
    char buf[MAXIMUM_PACKET_SIZE];
    if (0 != (rc = dgSocket.init (PORT_NUM))) {
        fprintf (stderr, "failed to bind datagram socket to port %d; rc = %d\n", PORT_NUM, rc);
        return -1;
    }
    while (true) {
        InetAddr clientAddr;
        if ((rc = dgSocket.receive (buf, sizeof (buf), &clientAddr)) <= 0) {
            fprintf (stderr, "failed to receive from datagram socket; rc = %d\n", rc);
            return -2;
        }
        dgSocket.sendTo (clientAddr.getIPAddress(), clientAddr.getPort(), buf, rc);
    }
}




int client (const char *pszServerIP, uint32 ui32InterPacketInterval, uint16 ui16PacketSize)
{
    int rc;
    char buf[MAXIMUM_PACKET_SIZE];
    if (0 != (rc = dgSocket.init (PORT_NUM))) {
        fprintf (stderr, "failed to bind datagram socket to port %d; rc = %d\n", PORT_NUM, rc);
        return -1;
    }
    printf ("inter-packet interval = %lu; packet size = %d\n", ui32InterPacketInterval, (int) ui16PacketSize);
    clientReceiverThread.start (clientReceiver, NULL);
    while (true) {
        *((int64*)buf) = getTimeInMilliseconds();
        if ((rc = dgSocket.sendTo (pszServerIP, PORT_NUM, buf, ui16PacketSize)) < 0) {
            fprintf (stderr, "failed to send datagram packet to %s:%d; rc = %d\n", pszServerIP, PORT_NUM, rc);
            //return -2;
        }
        else {
            ui32PacketsSent++;
        }
        sleepForMilliseconds (ui32InterPacketInterval);
    }
}

void clientReceiver (void *pArg)
{
    int rc;
    char buf[MAXIMUM_PACKET_SIZE];
    uint32 ui32PacketsReceived = 0;
    float fSRTT = 0.0;
    bool bNoRTTYet = true;
    const float ALPHA = 0.875;
    MovingAverage<int> mavg1(10), mavg2 (100);

    while (true) {
        if ((rc = dgSocket.receive (buf, MAXIMUM_PACKET_SIZE)) <= 0) {
            fprintf (stderr, "failed to receive from datagram socket; rc = %d\n", rc);
            //return;
        }
        else {
            ui32PacketsReceived++;
        }
        int64 i64SendTime = *((int64*)buf);
        uint16 ui16RTT = (uint16) (getTimeInMilliseconds() - i64SendTime);
        if (bNoRTTYet) {
            fSRTT = (float) ui16RTT;
            bNoRTTYet = false;
        }
        else {
            fSRTT = (ALPHA * fSRTT) + (1.0f - ALPHA) * ((float) ui16RTT);
        }
        mavg1.add (ui16RTT);
        mavg2.add (ui16RTT);
        printf ("Loss = %d%%; TCP-like RTT = %5.1f; MAvg(10) RTT = %5.1f; MAvg(100) RTT = %5.1f\n",
                ui32PacketsReceived == 0 ? 0 : (100 - ((ui32PacketsReceived * 100) / ui32PacketsSent)),
                fSRTT, (float) mavg1.getAverage(), (float) mavg2.getAverage());
    }
}
