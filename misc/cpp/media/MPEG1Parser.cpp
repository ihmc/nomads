/*
 * MPEG1Parser.cpp
 *
 * This file is part of the IHMC Misc Media Library
 * Copyright (c) 2015-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#include "MPEG1Parser.h"

#include "BufferReader.h"
#include "Logger.h"
#include "FileUtils.h"
#include "FileReader.h"

#include "MPEGElements.h"

#include "dvmbasic.h"
#include "dvmmpeg.h"

#include <assert.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

namespace IHMC_MISC
{
    class MPEG1ParserImpl : public MPEG1Parser
    {
        public:
            ~MPEG1ParserImpl (void);

            int64 getSize (void);

            int goToNextSequence (void);
            int goToNextSequenceGOP (void);
            int goToNextGOPFrame (void);
 
            int readAll (void *pBuf, unsigned int uiBufLen);
            int readToNextSequence (void *pBuf, unsigned int uiBufLen);
            int readToNextSequenceGOP (void *pBuf, unsigned int uiBufLen);
            int readToNextGOPFrame (void *pBuf, unsigned int uiBufLen);

            int readToEOF (void *pBuf, unsigned int uiBufLen);
            int reset (void);

        protected:
            explicit MPEG1ParserImpl (int64 i64Size);

        private:
            virtual int readBytes (void *pBuf, uint32 ui32Count) = 0;

        protected:
            BitStream *_bs;
            BitParser *_bp;

        private:
            enum State {
                OUT_OF_SEQUENCE,
                OUT_OF_GOP,
                IN_SEQUENCE,
                IN_GOP
            };

            const int64 _i64Size;
            int64 _i64Read;
            State _state;
            MpegSeqHdr *_sh;
            MpegPicHdr *_fh;
    };

    class MPEG1FileParserImpl : public MPEG1ParserImpl
    {
        public:
            MPEG1FileParserImpl (FILE *pFile, int64 i64Size);
            ~MPEG1FileParserImpl (void);

            int readBytes (void *pBuf, uint32 ui32Count);
            int seek (long ui64Pos);

        private:
            FileReader _fr;
    };

    class MPEG1BufferParserImpl : public MPEG1ParserImpl
    {
        public:
            MPEG1BufferParserImpl (const void *pBuf, uint32 ui32BufSize);
            ~MPEG1BufferParserImpl (void);

            int readBytes (void *pBuf, uint32 ui32Count);
            int seek (long ui64Pos);

        private:
            BufferReader _fr;
    };
}

using  namespace IHMC_MISC;

//-------------------------------------------------------------------

MPEG1Parser::MPEG1Parser (void)
{
}

MPEG1Parser::~MPEG1Parser (void)
{
}

//-------------------------------------------------------------------

MPEG1ParserImpl::MPEG1ParserImpl (int64 i64Size)
    : _bs (BitStreamNew (i64Size)),
      _bp (BitParserNew()),
      _i64Size (i64Size),
      _i64Read (0),
      _state (OUT_OF_SEQUENCE),
      _sh (MpegSeqHdrNew()),
      _fh (MpegPicHdrNew())
{
}

MPEG1ParserImpl::~MPEG1ParserImpl (void)
{
}

//-------------------------------------------------------------------

MPEG1FileParserImpl::MPEG1FileParserImpl (FILE *pFile, int64 i64Size)
    : MPEG1ParserImpl (i64Size),
      _fr (pFile, false)
{
    BitStreamFileRead (_bs, pFile, 0);
    BitParserWrap (_bp, _bs);
}

MPEG1FileParserImpl::~MPEG1FileParserImpl (void)
{
}

int MPEG1FileParserImpl::readBytes (void *pBuf, uint32 ui32Count)
{
    if (ui32Count == 0) {
        return 0;
    }
    return _fr.readBytes (pBuf, ui32Count);
}

int MPEG1FileParserImpl::seek (long ui64Pos)
{
    return _fr.seek (ui64Pos);
}

//-------------------------------------------------------------------

MPEG1BufferParserImpl::MPEG1BufferParserImpl (const void *pBuf, uint32 ui32BufSize)
    : MPEG1ParserImpl (ui32BufSize),
      _fr (_bs->buffer, ui32BufSize, false)
{
    memcpy (_bs->buffer, pBuf, ui32BufSize);
    _bs->size = ui32BufSize;
    _bs->endDataPtr = _bs->buffer + ui32BufSize;
    BitParserWrap (_bp, _bs);
}

MPEG1BufferParserImpl::~MPEG1BufferParserImpl (void)
{
}

int MPEG1BufferParserImpl::readBytes (void *pBuf, uint32 ui32Count)
{
    if (ui32Count == 0) {
        return 0;
    }
    return _fr.readBytes (pBuf, ui32Count);
}

int MPEG1BufferParserImpl::seek (long ui64Pos)
{
    return _fr.setPosition (ui64Pos);
}

//-------------------------------------------------------------------

MPEG1Parser * MPEG1ParserFactory::newParser (const char *pszFileName)
{
    if (pszFileName == NULL) {
       return NULL;
    }
    if (!FileUtils::fileExists (pszFileName)) {
        return NULL;
    }
    const int64 i64FileSize = FileUtils::fileSize (pszFileName);
    FILE *pfile = fopen (pszFileName, "rb");
    if (pfile == NULL) {
        return NULL;
    }
    MPEG1Parser *pParser = newParser (pfile, i64FileSize);
    if (pParser == NULL) {
        fclose (pfile);
        return NULL;
    }
    return pParser;
}

MPEG1Parser * MPEG1ParserFactory::newParser (FILE *pFile, int64 i64FileSize)
{
    if (i64FileSize <= 0) {
        return NULL;
    }
    return new MPEG1FileParserImpl (pFile, i64FileSize);
}

MPEG1Parser * MPEG1ParserFactory::newParser (const void *pBuf, uint32 ui32Size)
{
    if ((pBuf == NULL) || (ui32Size == 0)) {
        return NULL;
    }
    return new MPEG1BufferParserImpl (pBuf, ui32Size);
}

//-------------------------------------------------------------------

int MPEG1ParserImpl::goToNextSequence (void)
{
    const char *pszMethodName = "MPEG1Parser::goToNextSequence";
    int rc = MpegSeqHdrFind (_bp);
    if (rc == DVM_MPEG_NOT_FOUND) {
        return -1;
    }
    _state = IN_SEQUENCE;
    _i64Read += rc;
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "read %lld, while pos is %d.\n",
                    _i64Read, BitParserTell (_bp)); 
    return rc;
}

int64 MPEG1ParserImpl::getSize (void)
{
    return _i64Size;
}

int MPEG1ParserImpl::goToNextSequenceGOP (void)
{
    const char *pszMethodName = "MPEG1Parser::goToNextSequenceGOP";
    if (_state == OUT_OF_SEQUENCE) {
        return -1;
    }
    int rc = MpegGopHdrFind (_bp);
    if (rc == DVM_MPEG_NOT_FOUND) {
        return -2;
    }
    _state = IN_GOP;
    _i64Read += rc;
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "read %lld, while pos is %d.\n",
                    _i64Read, BitParserTell (_bp)); 
    return rc;
}

int MPEG1ParserImpl::goToNextGOPFrame (void)
{
    const char *pszMethodName = "MPEG1Parser::goToNextGOPFrame";
    if (_state != IN_GOP) {
        return -2;
    }
    int rc = MpegPicHdrFind (_bp);
    if (rc== DVM_MPEG_NOT_FOUND) {
        return -1;
    }
    assert ((_i64Read + rc) == BitParserTell (_bp));
    int tmp = MpegPicHdrParse (_bp, _fh);
    if (tmp < 0) {
        return -3;
    }
    rc += tmp;
    assert ((_i64Read + rc) == BitParserTell (_bp));
    int t = MpegPicHdrGetType (_fh);
    FrameType type = UNKNOWN;
    switch (t) {
        case I_FRAME: type = I; break;
        case P_FRAME: type = P; break;
        case B_FRAME: type = B; break;
        case D_FRAME: type = D; break;
        default: type = UNKNOWN;
    }
    int64 i64tmp = BitParserTell(_bp);
    tmp = MpegPicSkip (_bp);
    if (tmp < 0) {
        return -4;
    }
    i64tmp = BitParserTell (_bp) - i64tmp; 
    rc += i64tmp;
    assert ((_i64Read + rc) == BitParserTell (_bp));
    int currCode = MpegGetCurrStartCode (_bp);
    int iOffset = BitParserTell (_bp);
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "read %s frame with code %08x. Offset: %d\n",
                    toString (type), currCode, iOffset);
    if (currCode == GOP_START_CODE) {
        _state = OUT_OF_GOP;
    }
    if (currCode == SEQ_END_CODE) {
        _state = OUT_OF_SEQUENCE;
    }
    _i64Read += rc;
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "read %lld, while pos is %d.\n",
                    _i64Read, BitParserTell (_bp)); 
    return rc;
}

int MPEG1ParserImpl::readAll (void *pBuf, unsigned int uiBufLen)
{
    if ((pBuf == NULL) || (uiBufLen == 0)) {
        return -1;
    }
    if (_i64Size < 0) {
        return -2;
    }
    if (_i64Size > 0xFFFFFFFF) {
        return -3;
    }
    uint32 ui32Size = static_cast<uint32>(_i64Size);
    if ((ui32Size > 0) && (readBytes (pBuf, ui32Size) < 0)) {
        return -4;
    }
    return 0;
}

int MPEG1ParserImpl::readToNextSequence (void *pBuf, unsigned int uiBufLen)
{
    const char *pszMethodName = "MPEG1Parser::readToNextSequence";
    if ((pBuf == NULL) || (uiBufLen == 0)) {
        return -1;
    }
    int64 ui64CurrPos = BitParserTell (_bp);
    int iOff = goToNextSequence();
    if ((iOff < 0) || (iOff > uiBufLen)) {
        return -2;
    }
    if (seek (ui64CurrPos) < 0) {
        return -3;
    }
    if ((iOff > 0) && (readBytes (pBuf, iOff) < 0)) {
        return -4;
    }
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "reading %d bytes from %d (%u).\n",
                    iOff, ui64CurrPos, (static_cast<uint64>(iOff) + ui64CurrPos));
    return iOff;
}

int MPEG1ParserImpl::readToNextSequenceGOP (void *pBuf, unsigned int uiBufLen)
{
    const char *pszMethodName = "MPEG1Parser::readToNextSequenceGOP";
    if ((pBuf == NULL) || (uiBufLen == 0)) {
        return -1;
    }
    int64 ui64CurrPos = BitParserTell (_bp);
    int iOff = goToNextSequenceGOP();
    if ((iOff < 0) || (iOff > uiBufLen)) {
        return -2;
    }
    if (seek (ui64CurrPos) < 0) {
        return -3;
    }
    if ((iOff > 0) && (readBytes (pBuf, iOff) < 0)) {
        return -4;
    }
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "reading %d bytes from %d (%u).\n",
                    iOff, ui64CurrPos, (static_cast<uint64>(iOff) + ui64CurrPos));
    return iOff;
}

int MPEG1ParserImpl::readToNextGOPFrame (void *pBuf, unsigned int uiBufLen)
{
    const char *pszMethodName = "MPEG1Parser::readToNextGOPFrame";
    if ((pBuf == NULL) || (uiBufLen == 0)) {
        return -1;
    }
    int64 ui64CurrPos = BitParserTell (_bp);
    int iOff = goToNextGOPFrame();
    if ((iOff < 0) || (iOff > uiBufLen)) {
        return -2;
    }
    if (seek (ui64CurrPos) < 0) {
        return -3;
    }
    if ((iOff > 0) && (readBytes (pBuf, iOff) < 0)) {
        return -4;
    }
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "reading %d bytes from %d (%u).\n",
                    iOff, ui64CurrPos, (static_cast<uint64>(iOff) + ui64CurrPos));
    return iOff;
}

int MPEG1ParserImpl::readToEOF (void *pBuf, unsigned int uiBufLen)
{
    if (_i64Size > _i64Read) {
        int64 i64Diff = _i64Size - _i64Read;
        if (i64Diff > uiBufLen) {
            return -1;
        }
        if (seek (_i64Read) < 0) {
            return -2;
        }
        if ((i64Diff > 0) && (readBytes (pBuf, i64Diff) < 0)) {
            return -3;
        }
        return static_cast<int>(i64Diff);
    }
    return 0;
}

int MPEG1ParserImpl::reset (void)
{
    BitParserSeek (_bp, 0);
    _i64Read = 0;
    _state = OUT_OF_SEQUENCE;
    seek (0);
    return 0;
}

