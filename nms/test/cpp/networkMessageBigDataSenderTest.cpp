#include <stdio.h>
#include "NetworkMessageService.h"
#include "TestListener.h"
#include "FileReader.h"
#include "Logger.h"

#include "NLFLib.h"
#include "NetUtils.h"
#include <stdlib.h>
#define MAXREAD 64000

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
    TestListener tList1;

    pLogger = new Logger();
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel(Logger::L_HighDetailDebug);

    NetworkMessageService *netMsgSrv = new NetworkMessageService;

    printf("Test: init\n");
    netMsgSrv->init(NetworkMessageService::DEFAULT_PORT);
    netMsgSrv->registerHandlerCallback(4, &tList1);

    char* pTestMess = new char[MAXREAD];
    printf("Test: broadcast\n");

    FILE * file = fopen ( "./image1.jpg", "r" );
    printf("fd: %p\n", file);
    FileReader f (file, true);

    int nread = f.read(pTestMess,MAXREAD);
    printf("read %d bytes\n", nread);

    netMsgSrv->broadcastMessage (4, NULL, INADDR_BROADCAST, NULL, 0, 3, 5000, NULL, 0, pTestMess, nread, false);
    f.close();

    delete netMsgSrv;

    return 0;         
}
