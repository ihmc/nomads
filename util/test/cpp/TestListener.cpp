#include "TestListener.h"

#include "NLFLib.h"

#include <stdio.h>
#include <stdlib.h>

TestListener::TestListener (void)
{
    _ui64DataMsgBytesRcvd = 0;
}

TestListener::~TestListener (void)
{
}

int TestListener::messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                        uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                        const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                        const void *pMsg, uint16 ui16MsgLen)
{
    char *pszMess = (char*) malloc (ui16MsgLen*sizeof(char));
    for (int iCount=0;iCount<ui16MsgLen;iCount++) {
        pszMess[iCount] = ((char*)pMsg)[iCount];
    }
    pszMess[ui16MsgLen]= '\0';

    printf ("TestListener::messageArrived: %d bytes received: \"%s\" from interface: %s\n", ui16MsgLen, pszMess, pszIncomingInterface);

    uint8 ui8Type = (uint8) atoui32 ((const char *)pMsgMetaData);
    switch (ui8MsgType) {
        case DSMT_Data:
            printf ("data");
             _ui64DataMsgBytesRcvd += ui16MsgLen;
            printf ("TestListener::messageArrived: total bytes received is %u\n\n", _ui64DataMsgBytesRcvd);
            break;

        case DSMT_DataReq:
            printf ("data request");
            break;

        case DSMT_WorldStateSeqId:
            printf ("world state seq ID");
            break;

        case DSMT_SubStateMessage:
            printf ("subscription state");
            break;

        case DSMT_SubStateReq:
            printf ("subscription state request");
            break;

	   case DSMT_DataCacheQuery:
            printf ("data cache query");
            break;

        case DSMT_DataCacheQueryReply:
            printf ("data cache query reply");
            break;

        case DSMT_DataCacheMessagesRequest:
            printf ("data cache message request");
            break;

        case DSMT_AcknowledgmentMessage:
            printf ("ack");
            break;

        case DSMT_CompleteMessageReq:
            printf ("complete message request");

        case DSMT_CacheEmpty:
            printf ("cache empty");

        default:
            printf ("unknown");
            break;
    }

    return 0;
}
