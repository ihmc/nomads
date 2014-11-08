#include "Mocket.h"
#include "MessageSender.h"

#include "Logger.h"

using namespace NOMADSUtil;

int main (int argc, char *argv[])
{
    int rc;
    pLogger = new Logger();
    pLogger->initLogFile ("TestClient.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);

    Mocket mocket;
    if (0 != (rc = mocket.connect (argv[1], 1973))) {
        printf ("failed to connect to IP %s; rc = %d\n", argv[1], rc);
        return -1;
    }
    printf ("sending data\n");
    MessageSender ms = mocket.getSender (true, true);
    ms.send ("hello, world 0", 14);
    ms.send ("hello, world 1", 14);
    ms.send ("hello, world 2", 14);
    ms.send ("hello, world 3", 14);
    ms.send ("hello, world 4", 14);
    ms.send ("hello, world 5", 14);
    ms.send ("hello, world 6", 14);
    ms.send ("hello, world 7", 14);
    ms.send ("hello, world 8", 14);
    ms.send ("hello, world 9", 14);
    printf ("sent data - closing connection\n");
    mocket.close();
    printf ("closed connection\n");
    return 0;
}

/*
 * vim: et ts=4 sw=4
 */
