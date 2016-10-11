#include <stdio.h>
#include <stdlib.h>

#include "DArray.h"
#include "StringTokenizer.h"
#include "UDPDatagramSocket.h"
#include "UDPRawDatagramSocket.h"

#if defined (UNIX)
    #define stricmp strcasecmp
#endif

#include "InetAddr.h"

using namespace NOMADSUtil;

struct IPAddress   // Needed because DArray<uint32> does not work!
{
    uint32 ui32Addr;
};

int main (int argc, char *argv[])
{
    int rc;

    uint32 ui32ReceiveAddr = 0;   // This is to receive UDP Packets to be re-broadcast
    uint16 ui16ReceivePort = 0;   // This is to receive UDP Packets to be re-broadcast

    uint32 ui32LocalSendAddr = 0; // This is to identify the local interface that will be used to re-broadcast the UDP Packets

    DArray<IPAddress> udpRelayAddrs;
    
    int i = 1;
    while (i < argc) {
        if ((0 == stricmp (argv[i], "-receiveAddr")) || (0 == stricmp (argv[i], "-ra"))) {
            i++;
            if (i < argc) {
                ui32ReceiveAddr = InetAddr (argv[i]).getIPAddress();
                fprintf (stdout, "set receive address as <%s>\n", argv[i]);
            }
        }
        else if ((0 == stricmp (argv[i], "-receivePort")) || (0 == stricmp (argv[i], "-rp"))) {
            i++;
            if (i < argc) {
                ui16ReceivePort = (uint16) atoi (argv[i]);
                fprintf (stdout, "set receive port to <%d>\n", (int) ui16ReceivePort);
            }
        }
        else if ((0 == stricmp (argv[i], "-localSendAddr")) || (0 == stricmp (argv[i], "-lsa"))) {
            i++;
            if (i < argc) {
                ui32LocalSendAddr = InetAddr (argv[i]).getIPAddress();
                fprintf (stdout, "using interface <%s> to rebroadcast packets\n", argv[i]);
            }
        }
        else if ((0 == stricmp (argv[i], "-udpTargetAddrs")) || (0 == stricmp (argv[i], "-udps"))) {
            i++;
            if (i < argc) {
                StringTokenizer st (argv[i], ',');
                const char *pszAddr;
                while (NULL != (pszAddr = st.getNextToken())) {
                    IPAddress udpAddr;
                    udpAddr.ui32Addr = InetAddr (pszAddr).getIPAddress();
                    fprintf (stdout, "adding <%s> as a UDP relay address\n", pszAddr);
                    udpRelayAddrs[udpRelayAddrs.size()] = udpAddr;
                }
            }
        }
        i++;
    }
    
    if (ui16ReceivePort == 0) {
        // Must at least have the receive port
        fprintf (stdout, "usage: %s [-receiveAddr <receiveAddr>] -receivePort <receivePort> [-localSendAddr <sendAddr>] [-udpTargetAddrs <addr>[,<addr>...]]\n", argv[0]);
        return -1;
    }

    UDPDatagramSocket dgRecv;
    UDPRawDatagramSocket dgSend;
    
    if (0 != (rc = dgRecv.init (ui16ReceivePort, ui32ReceiveAddr))) {
        fprintf (stderr, "failed to bind the receive socket\n");
        return -2;
    }
    if (0 != (rc = dgSend.init (0, ui32LocalSendAddr))) {
        fprintf (stderr, "failed to bind the send socket - make sure you run this with root/administrative privileges\n");
        return -3;
    }
    
    char buf[2048];
    uint32 ui32Counter = 0;
    while (true) {
        InetAddr senderAddr;
        rc = dgRecv.receive (buf, sizeof (buf), &senderAddr);
        if (rc <= 0) {
            fprintf (stderr, "failed to receive from socket; rc = %d\n", rc);
            sleepForMilliseconds (500);
        }

        if (rc <= 6) {
            printf ("received a short packet %lu of size %d bytes - ignoring\n", ++ui32Counter, rc);
            continue;
        }
        else {
            printf ("Received packet %lu of size %d bytes - relaying\n", ++ui32Counter, rc);
        }

        uint32 ui32MsgSize = (uint32) (rc - 6);

        // First four bytes is the address to which the packet should be sent
        uint32 ui32DestAddr;
        memcpy (&ui32DestAddr, buf, sizeof (ui32DestAddr));

        // Next two bytes is the port to which the packet should be sent
        uint16 ui16DestPort;
        memcpy (&ui16DestPort, buf+4, sizeof (ui16DestPort));

        // Resend the packet pretending to be from the original sender
        if ((rc = dgSend.sendTo (senderAddr.getIPAddress(), senderAddr.getPort(), ui32DestAddr, ui16DestPort, buf+6, ui32MsgSize)) <= 0) {
            fprintf (stderr, "failed to send on the send socket; rc = %d\n", rc);
        }

        // Now send the packet to all the UDP relays
        for (uint32 ui32 = 0; ui32 < udpRelayAddrs.size(); ui32++) {
            if ((rc = dgSend.sendTo (senderAddr.getIPAddress(), senderAddr.getPort(), udpRelayAddrs[ui32].ui32Addr, ui16DestPort, buf+6, ui32MsgSize)) <= 0) {
                fprintf (stderr, "failed to send to a UDP relay address; rc = %d\n", rc);
            }
        }
    }
    return 0;
}

