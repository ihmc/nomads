#include "NetworkMessageService.h"
#include "TestListener.h"
#include "NLFLib.h"
#include "StrClass.h"

#include <stdio.h>
#include <stdlib.h>

#if defined (WIN32)
    #define _WINSOCKAPI_ //prevent inclusion of winsock.h
    #include <winsock2.h>
#elif defined (UNIX)
    #include <arpa/inet.h>
    #include <stdlib.h>
#endif

using namespace NOMADSUtil;

int main (int argc, char **argv)
{

    //fare le net interface

	TestListener tList1;
    TestListener tList2;
    TestListener tList3;

    NetworkMessageService *netMsgSrv = new NetworkMessageService;
    const char **interfaces = (const char**) malloc (16*2*sizeof(char));

    if (argc > 1) {
        interfaces[0] = argv[1];
        interfaces[1] = NULL;
    }
    else {
        interfaces[0] = NULL;
    }

    printf("Test: init\n");
    netMsgSrv->init(NetworkMessageService::DEFAULT_PORT,interfaces[0]);
    netMsgSrv->registerHandlerCallback(4,&tList1);
    netMsgSrv->registerHandlerCallback(7,&tList1);
    netMsgSrv->registerHandlerCallback(4,&tList3);
   
    const char* pTestMess = new char[10];

    printf("Test: broadcast\n");

    String msg = "Message 1";
    netMsgSrv->broadcastMessage(100,				// Msg Type
                                interfaces, 		// Outgoing interfaces
								INADDR_BROADCAST, 	// Target Address
								0, 					// MsgId
								0, 					// HopCount
								3,					// TTL
								5000, 				// Delay Tolerance
							    msg.c_str(),        // Metadata
								msg.length(),		// Metadata Length
								msg.c_str(),		// Data
								msg.length(),		// DataLen
								false);				// bExpedited
    msg = "Message 2";
    netMsgSrv->broadcastMessage(5, interfaces, INADDR_BROADCAST, 0, 0, 3, 5000, msg.c_str(), msg.length(), msg.c_str(), msg.length(), false);

    msg = "Message 3";
    netMsgSrv->broadcastMessage(1, interfaces, INADDR_BROADCAST, 0, 0, 3, 5000, msg.c_str(), msg.length(), msg.c_str(), msg.length(), false);

    msg = "Message 4";
    netMsgSrv->broadcastMessage(2, interfaces, INADDR_BROADCAST, 0, 0, 3, 5000, msg.c_str(), msg.length(), msg.c_str(), msg.length(), false);

    msg = "Message 5";
    netMsgSrv->broadcastMessage(4, interfaces, INADDR_BROADCAST, 0, 0, 3, 5000, msg.c_str(), msg.length(), msg.c_str(), msg.length(), false);

	delete netMsgSrv;
	free ((char **)interfaces);

    return 0;         
}
