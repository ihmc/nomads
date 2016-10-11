#include <stdio.h>
#include <stdlib.h>

#include "ArgParse.h"

#include "InetAddr.h"
#include "TCPSocket.h"
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

    const Arguments args (ArgParse::parseReceiverArgs (argc, argv));
    TCPSocket tcps;
    int rc = tcps.setupToReceive (args._ui16SrcPort, 1, args._ui32SrcAddr);
    if (rc < 0) {
        fprintf (stderr, "error: could not bind TCP socket to <%s>:<%d>; rc = %d\n",
                 InetAddr (args._ui32SrcAddr).getIPAsString(), (int) args._ui16SrcPort, rc);
        return -3;
    }
    Socket *pSock = tcps.accept();
    if (pSock == NULL) {
        return -4;
    }

    char *printBuf = NULL;
    uint8 *pui8Buf = static_cast<uint8*>(calloc (args._ui16PktLen, sizeof (uint8)));
    const uint16 ui16BufLen = args._ui16PktLen;
    if (args._bPrntData) {
        uint32 ui32PrintBufSize = 3 * ui16BufLen + 1;             // 2 characters to print HEX value and one space for each byte, and the final '\0' character
        printBuf = new char[ui16BufLen];
        memset (printBuf, 0, ui16BufLen);
    }
    Metrics currMetrics;
    ReceiverMetrics metrics (1000);
    PacketFactory pkt (args._bUseSeqNum, args._bUseRelTime);
    InetAddr senderAddr;
    uint32 ui32SeqNum;
    int64 i64SendingTime;
    while (true) {
        if ((rc = pSock->receiveBytes (pui8Buf, ui16BufLen)) < 0) {
            fprintf (stderr, "error: receive() on TCP socket failed; rc = %d\n", rc);
            return -3;
        }

        if (pkt.extractHeaders (pui8Buf, ui16BufLen, ui32SeqNum, i64SendingTime) == 0) {
            //printf ("received a packet of size %d from %s:%d Packet sequence number %d from %s\n",
              //      rc, senderAddr.getIPAsString(), (int) senderAddr.getPort(), ui32SeqNum, udps.getLocalAddr().getIPAsString());
        }
        if (metrics.packetArrived (rc, ui32SeqNum, i64SendingTime, currMetrics)) {
            printf ("Received packet with seq id %u sent at time %lld from node %s\n",
                    ui32SeqNum, i64SendingTime, pSock->getRemoteHostAddr());
            currMetrics.display();
            printf ("\n");
        }

        if (args._bPrntData) {
            int i = 0;
            char *bufPointer = printBuf;
            printf ("Data:\n");
            bufPointer += sprintf (bufPointer, "%02X ", pui8Buf[i++]);
            for (; i < rc; i++) {
                if ((i % 16) == 0) {
                    bufPointer += sprintf (--bufPointer, "\n");
                }
                bufPointer += sprintf (bufPointer, "%02X ", pui8Buf[i]);
            }
            *(--bufPointer) = '\0';
            printf ("%s\n", printBuf);
        }
    }
    return 0;
}
