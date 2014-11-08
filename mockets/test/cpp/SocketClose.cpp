/*
 * Created to test how sockets behave when closing and when the connection is dropped while closing
 *  
 * File:   SocketClose.cpp
 */

#include <cstdlib>
#include "TCPSocket.h"
#include "Thread.h"
#include "NLFLib.h"



#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

using namespace NOMADSUtil;
using namespace std;
/*
 * 
 */

class CloserThread : public Thread
{
    public:
        CloserThread (TCPSocket *socket);
        void run (void);

    private:
        TCPSocket *_psocket;
};

CloserThread::CloserThread (TCPSocket *socket)
{
    _psocket = socket;
}

void CloserThread::run (void)
{
    sleepForMilliseconds (500);
    printf ("6\n");
    int rc = _psocket->disconnect();
    printf ("7\n");
    printf ("rc = %d\n", rc);
}

int main(int argc, char** argv)
{
    bool bClient;
    String remoteHost;
    uint16 ui16Port = 1234;
    
    remoteHost = argv[1];
    printf ("1\n");
    
    TCPSocket *socket = new TCPSocket();
    printf ("2\n");
    CloserThread *ct = new CloserThread(socket);
    printf ("3\n");
    ct->start();
    printf ("4\n");
    
    socket->connect (remoteHost, ui16Port);
    printf ("5\n");
    //socket->disconnect();
    while (true) {
        sleepForMilliseconds (1000);
    }

    return 0;
}

