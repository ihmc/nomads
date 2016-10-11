#include <stdio.h>
#include <stdlib.h>

#include "NetworkMessageService.h"
#include "TestListener.h"
#include "UDPDatagramSocket.h"
#include "NLFLib.h"
#include "NetworkMessageServiceListener.h"
#include "Logger.h"

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
    pLogger = new Logger();
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel(Logger::L_HighDetailDebug);
    
    TestListener tList1;
    
    NetworkMessageService netMsgSrv;
	
	if (argc>1) netMsgSrv.init(NetworkMessageService::DEFAULT_PORT,argv[1]);
        else netMsgSrv.init(NetworkMessageService::DEFAULT_PORT);

	netMsgSrv.registerHandlerCallback(4, &tList1);

	while (true)
	{
	    sleepForMilliseconds(50000);
	}
    

    
    return 0;
}

