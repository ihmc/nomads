#include <stdio.h>
#include <stdlib.h> //atoi
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "MulticastUDPDatagramSocket.h"

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
    if (argc != 5) {
        fprintf (stderr, "usage: %s <if_addr> <port> <multicast_group> <multicast_net_if>\n", argv[0]);
        exit (-1);
    }
    
    uint32 ui32ListenAddr = inet_addr (argv[1]);
    uint16 ui16Port = atoi (argv[2]);
    uint32 ui32MulticastGroup = inet_addr (argv[3]);
    uint32 ui32MulticastNetIF = inet_addr (argv[4]);

    MulticastUDPDatagramSocket sock;
    if (0 != sock.init(ui16Port, ui32ListenAddr)) {
        fprintf (stderr, "error initializing socket\n");
        return -1;
    }

    if (0 != sock.joinGroup(ui32MulticastGroup, ui32MulticastNetIF)) {
        fprintf (stderr, "error joining group\n");
        return -2;
    }

    printf ("waiting for connections...\n");
    int iLastPackSeqNo = 0;
    while (true) {
        int iPacketSize;
        InetAddr remoteAddr;
        char pchPacket [65000];
        if ((iPacketSize = sock.receive(pchPacket, sizeof(pchPacket), &remoteAddr)) > 0) {
            int iPackSeqNo = atoi (pchPacket);
            printf ("received packet#: %d from %s\n", iPackSeqNo, (const char*) remoteAddr.getIPAsString());
            if ((iLastPackSeqNo != 0) && ((iPackSeqNo - 1) != iLastPackSeqNo)) {
                printf ("-----> lost packet#: %d\n", (iPackSeqNo - 1));
            }
            iLastPackSeqNo = iPackSeqNo;
        }
        else {
            printf ("error occurred\n");
        }
    }

    return 0;
}

