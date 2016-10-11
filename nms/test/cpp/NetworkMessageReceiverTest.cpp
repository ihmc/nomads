#include <stdio.h>
#include <stdlib.h>

#include "NetworkMessageService.h"
#include "NetworkMessageServiceListener.h"
#include "NLFLib.h"
#include "TestListener.h"
#include "UDPDatagramSocket.h"

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
    TestListener tListener;
    NetworkMessageService netMsgSrv;
    netMsgSrv.init(NetworkMessageService::DEFAULT_PORT);

    netMsgSrv.registerHandlerCallback (100, &tListener);

    while (true) {
        sleepForMilliseconds (1000);
    }

    return 0;
}
