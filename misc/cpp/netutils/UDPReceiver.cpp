#include <stdio.h>
#include <stdlib.h>

#include "InetAddr.h"
#include "UDPDatagramSocket.h"
#include "PacketFactory.h"
#include "ReceiverMetrics.h"

using namespace NOMADSUtil;
using namespace IHMC_MISC;

int main (int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf (stderr, "usage: %s [-useSeqNum] [-useRelTime] [-listenIP <ListenIPAddr>] [-printData] <ListenPort>\n",
                 argv[0]);
        return -1;
    }

    const char *pszListenIP = NULL;
    const char *pszListenPort = argv[argc-1];
    bool bUseSeqNum = false;
    bool bUseRelTime = false;
    bool bEnableMetrics = false;
    bool bPrintData = false;

    for (int i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], "-useSeqNum")) {
            bUseSeqNum = true;
        }
        else if (0 == strcmp(argv[i], "-useRelTime")) {
            bUseRelTime = true;
        }
        else if (0 == strcmp (argv[i], "-listenIP")) {
            pszListenIP = argv[++i];
        }
        else if (0 == strcmp (argv[i], "-printData")) {
            bPrintData = true;
        }
    }

    if (bUseSeqNum) {
        printf ("Using sequence numbered packets\n");
    }
    if (bUseRelTime) {
        printf ("Using relative timestamped packets\n");
    }
    if (pszListenIP) {
        printf ("ListenIPAddr set to %s\n", pszListenIP);
    }
    if (bPrintData) {
        printf ("Data contained in received UDP datagrams will be printed on console\n");
    }

    UDPDatagramSocket udps;

    uint32 ui32ListenIP = 0;
    if (pszListenIP) {
        ui32ListenIP = InetAddr(pszListenIP).getIPAddress();
    }
    uint16 ui16ListenPort = (uint16) atoi (pszListenPort);

    int rc;
    if (0 != (rc = udps.init (ui16ListenPort, ui32ListenIP))) {
        fprintf (stderr, "error: could not bind UDP socket to <%s>:<%d>; rc = %d\n",
                 InetAddr(ui32ListenIP).getIPAsString(), (int) ui16ListenPort, rc);
        return -2;
    }

    char *printBuf = NULL;
    uint8 ui8Buf[65535];
    const uint16 ui16BufLen = sizeof (ui8Buf);
    if (bPrintData) {
        uint32 ui32PrintBufSize = 3*65535 + 1;             // 2 characters to print HEX value and one space for each byte, and the final '\0' character
        printBuf = new char[65536];
        memset (printBuf, 0, 65536);
    }
    Metrics currMetrics;
    ReceiverMetrics metrics (1000);
    PacketFactory pkt (bUseSeqNum, bUseRelTime);
    InetAddr senderAddr;
    uint32 ui32SeqNum;
    int64 i64SendingTime;
    while (true) {
        if ((rc = udps.receive (ui8Buf, ui16BufLen, &senderAddr)) < 0) {
            fprintf (stderr, "error: receive() on UDP socket failed; rc = %d\n", rc);
            return -3;
        }

        if (pkt.extractHeaders (ui8Buf, ui16BufLen, ui32SeqNum, i64SendingTime) == 0) {
            //printf ("received a packet of size %d from %s:%d Packet sequence number %d from %s\n",
              //      rc, senderAddr.getIPAsString(), (int) senderAddr.getPort(), ui32SeqNum, udps.getLocalAddr().getIPAsString());
        }
        if (metrics.packetArrived (rc, ui32SeqNum, i64SendingTime, currMetrics)) {
            printf ("Received packet with seq id %u sent at time %lld from node %s\n",
                    ui32SeqNum, i64SendingTime, udps.getLocalAddr().getIPAsString());
            currMetrics.display();
            printf ("\n");
        }

        if (bPrintData) {
            int i = 0;
            char *bufPointer = printBuf;
            printf ("Data:\n");
            bufPointer += sprintf (bufPointer, "%02X ", ui8Buf[i++]);
            for (; i < rc; i++) {
                if ((i % 16) == 0) {
                    bufPointer += sprintf (--bufPointer, "\n");
                }
                bufPointer += sprintf (bufPointer, "%02X ", ui8Buf[i]);
            }
            *(--bufPointer) = '\0';
            printf ("%s\n", printBuf);
        }
    }
    return 0;
}
