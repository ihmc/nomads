/* 
 * File:   PacketFactory.h
 * Author: gbenincasa
 *
 * Created on May 12, 2015, 12:29 PM
 */

#ifndef INCL_PACKET_FACTORY_H
#define	INCL_PACKET_FACTORY_H

#include "BufferWriter.h"
#include "FTypes.h"

namespace IHMC_MISC
{
    class PacketFactory
    {
        public:
            PacketFactory (bool bUseSeqNum, bool bUseRelTime);
            virtual ~PacketFactory (void);

            int extractHeaders (uint8 *pui8Buf, uint16 ui16Len,
                                uint32 &ui32SeqId, int64 &i64RelTime);

            void updateHeader (void);
            int init (uint8 *pui8Buf, uint16 ui16Len);

        private:
            const bool _bUseSeqNum;
            const bool _bUseRelTime;
            uint32 _ui32SeqNum;
            NOMADSUtil::BufferWriter _bw;
    };
}

#endif	/* INCL_PACKET_FACTORY_H */

