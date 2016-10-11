#include "net/NetworkMessageServiceListener.h"

using namespace NOMADSUtil;

class TestListener : public NetworkMessageServiceListener
{
    public:
        TestListener ();
        virtual ~TestListener (void);

         enum Type {
                DSMT_Unknown = 0x00,
                DSMT_Data = 0x01,
                DSMT_DataReq = 0x02,
                DSMT_WorldStateSeqId = 0x03,
                DSMT_SubStateMessage = 0x04,
                DSMT_SubStateReq = 0x05,
                DSMT_TopologyStateMessage = 0x06,
                DSMT_DataCacheQuery = 0x07,
                DSMT_DataCacheQueryReply = 0x08,
                DSMT_DataCacheMessagesRequest = 0x09,
                DSMT_AcknowledgmentMessage = 0x10,
                DSMT_CompleteMessageReq = 0x11,
                DSMT_CacheEmpty = 0x12,
                DSMT_CtrlToCtrlMessage = 0x13,
                DSMT_ChunkReq = 0x14,
                DSMT_HistoryReq = 0x15,
                DSMT_HistoryReqReply = 0x16,
        };

        virtual int messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                        uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                        const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                        const void *pMsg, uint16 ui16MsgLen);

    private:
        uint64 _ui64DataMsgBytesRcvd;
};
