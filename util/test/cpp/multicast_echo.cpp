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


#define MULTICAST_GROUP "225.1.2.3"
#define TIMEOUT 3000
#define PORT    5000

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
    uint32 ui32MulticastGroup = inet_addr (MULTICAST_GROUP);
    MulticastUDPDatagramSocket ms;
    ms.init (5000, ui32MulticastGroup);
    ms.joinGroup (ui32MulticastGroup);
    ms.setLoopbackMode (false);
    ms.setTTL (2);

    if (fork()>0) {
        char message[256];
        struct utsname name;
        uname (&name);
        sprintf (message, "Hello from: %s", name.nodename);
        while (true) {
            printf ("Sending multicast: %s\n", message);
            ms.sendTo (ui32MulticastGroup, PORT, message, strlen (message)+1);
            sleepForMilliseconds (TIMEOUT);
        }  
    }
    else { // child
        char buf[256];
        InetAddr remoteAddr;
        for (;;) {
            ms.receive (buf, sizeof (buf), &remoteAddr);
            printf ("Received multicast: %s\n", buf);
        }
    }

    return 0;         
}

