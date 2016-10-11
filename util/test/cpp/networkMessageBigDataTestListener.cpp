#include "TestListener.h"
#include <stdio.h>
#include <stdlib.h>
#include "FileWriter.h"
#include "NLFLib.h"

TestListener::TestListener ()
{
	
}
TestListener::~TestListener (void)
{
}

int TestListener::messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                    uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                    const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                    const void *pMsg, uint16 ui16MsgLen)
{

	printf("BigDataTestlistener: %d bytes received\n", ui16MsgLen);
    FileWriter fw ( "./image1_remote.jpg", "w" );
    printf("writing %d bytes (returned:%d)\n", ui16MsgLen, fw.writeBytes(pMsg,(unsigned long)ui16MsgLen));

    fw.flush();
    fw.close();

    return 0;
}

