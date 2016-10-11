#include <stdio.h>
#include "MulticastUDPDatagramSocket.h"
#include "NLFLib.h"

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
    if (argc != 6) {
        fprintf (stderr, "usage: %s <port> <iface_addr> <multicast_group> <multicast_ttl> <interval>\n", argv[0]);
        exit (-1);
    }

    uint16 ui16Port = atoi (argv[1]);
    uint32 ui32IFAddr = inet_addr (argv[2]);
    uint32 ui32MulticastGroup = inet_addr (argv[3]);
    uint8 ui8MulticastTTL = atoi (argv[4]);
    uint32 ui32Interval = atoi (argv[5]);

    MulticastUDPDatagramSocket ms;
    ms.init (ui16Port, ui32IFAddr);
    ms.joinGroup (ui32MulticastGroup);
    ms.setLoopbackMode (false);
    ms.setTTL (ui8MulticastTTL);

    char message[256];
    struct utsname name;
    uname (&name);
    int iSeqNo = 0;
    while (true) {
        sprintf (message, "%d", iSeqNo++);
        printf ("Sending packet#: %s\n", message);
        ms.sendTo (ui32MulticastGroup, ui16Port, message, strlen (message)+1);
        sleepForMilliseconds (ui32Interval);
    }  

    return 0;         
}

