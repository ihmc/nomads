/* 
 * File:   PacketFactory.cpp
 * Author: gbenincasa
 * 
 * Created on May 12, 2015, 12:29 PM
 */

#include "PacketFactory.h"

#include "BufferReader.h"
#include "EndianHelper.h"
#include "NLFLib.h"
#include "Reader.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

PacketFactory::PacketFactory (bool bUseSeqNum, bool bUseRelTime)
    : _bUseSeqNum (bUseSeqNum),
      _bUseRelTime (bUseRelTime),
      _ui32SeqNum (0U)
{
}

PacketFactory::~PacketFactory (void)
{
}

int PacketFactory::extractHeaders (uint8 *pui8Buf, uint16 ui16Len,
                                   uint32 &ui32SeqId, int64 &i64RelTime)
{
    BufferReader br (pui8Buf, ui16Len);
    if (_bUseSeqNum) {
        if (ui16Len < 4) {
            return -1;
        }
        br.read32 (&ui32SeqId);
    }
    if (_bUseRelTime) {
        br.read64 (&i64RelTime);
    }
    return 0;
}

int PacketFactory::init (uint8 *pui8Buf, uint16 ui16Len)
{
    _bw.init ((char *)pui8Buf, 12);
    updateHeader();
    uint16 ui16Pos = 4U + 8U;
    for (; ui16Pos < ui16Len; ui16Pos++) {
        pui8Buf[ui16Pos] = static_cast<uint8>(ui16Pos % 255);
    }
    return 0;
}

void PacketFactory::updateHeader (void)
{    
    _bw.reset();
    if (_bUseSeqNum) {
        _bw.write32 (&_ui32SeqNum);
        _ui32SeqNum++;
    }
    if (_bUseRelTime) {
        int64 i64SendingTime = getTimeInMilliseconds();
        _bw.write64 (&i64SendingTime);
    }
}
