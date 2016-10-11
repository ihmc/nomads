/*
* ImageCodec.cpp
*
* This file is part of the IHMC Misc Library
* Copyright (c) 2010-2016 IHMC.
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

#include "ImageCodec.h"

#include "ChunkingUtils.h"
#include "Defs.h"
#ifndef ANDROID
    #include "JasperWrapper.h"
    #include "PNGLibWrapper.h"
#endif
#include "JPEGLibWrapper.h"

#include "BMPImage.h"
#include "BufferReader.h"

#include "Logger.h"

#define encodingErrMsg Logger::L_MildError, "failed to encode BMP chunk into %s.\n"
#define decodingErrMsg Logger::L_MildError, "failed to decode %s chunk into BMP.\n"
#define unsupportedType(io,type) Logger::L_MildError, "unsupported %s type %d\n", io, static_cast<int>(type)

using namespace IHMC_MISC;
using namespace NOMADSUtil;

namespace IHMC_MISC
{
    typedef BMPImage*(*BMPDecodingFnPtr) (const void *pInputBuf, uint32 ui32InputBufLen);
    typedef BufferReader*(*BMPEncodingFnPtr) (const BMPImage *pBMPImage);

    BMPImage * toImg (const void *pInputBuf, uint32 ui32InputBufLen)
    {
        BufferReader br (pInputBuf, ui32InputBufLen, false);
        return IHMC_MISC::ChunkingUtils::toBMPImage (&br);
    }

    BMPImage * decodeImg (const void *pInputBuf, uint32 ui32InputBufLen, const char *pszInputType, BMPDecodingFnPtr pDecode)
    {
        BMPImage *pBMPImage = pDecode (pInputBuf, ui32InputBufLen);
        if (pBMPImage == NULL) {
            checkAndLogMsg ("Chunker::decodeImg", decodingErrMsg, pszInputType);
        }
        return pBMPImage;
    }

    BufferReader * encodeImg (const BMPImage *pBMPImage, const char *pszOutputType, BMPEncodingFnPtr pEncode)
    {
        BufferReader *pReader = pEncode (pBMPImage);
        if (pReader == NULL) {
            checkAndLogMsg ("Chunker::encodeImg", encodingErrMsg, pszOutputType);
        }
        return pReader;
    }
}

BMPImage * ImageCodec::decode (const void *pBuf, uint32 ui32Len, Chunker::Type inputObjectType)
{
    if (inputObjectType == Chunker::BMP) {
        return decodeImg (pBuf, ui32Len, "BMP", &toImg);
    }
    if (inputObjectType == Chunker::JPEG) {
        return decodeImg (pBuf, ui32Len, "JPEG", &JPEGLibWrapper::convertJPEGToBMP);
    }
#ifndef ANDROID
    if (inputObjectType == Chunker::JPEG2000) {
        return decodeImg (pBuf, ui32Len, "JPEG2000", &JasperWrapper::convertToBMPAsImg);
    }
    if (inputObjectType == Chunker::PNG) {
        return decodeImg (pBuf, ui32Len, "PNG", &PNGLibWrapper::convertPNGToBMP);
    }
#endif
    checkAndLogMsg ("Chunker::decode", unsupportedType ("input", inputObjectType));
    return NULL;
}

BufferReader * ImageCodec::encode (const BMPImage *pBMPImage, Chunker::Type outputChunkType, uint8 ui8ChunkCompressionQuality)
{
    const char *pszMethodName = "Chunker::encode";
    if (outputChunkType == Chunker::BMP) {
        return encodeImg (pBMPImage, "BMP", &ChunkingUtils::toReader);
    }
    if (outputChunkType == Chunker::JPEG) {
        BufferReader *pReader = JPEGLibWrapper::convertBMPToJPEG (pBMPImage, ui8ChunkCompressionQuality);
        if (pReader == NULL) {
            checkAndLogMsg (pszMethodName, encodingErrMsg, "JPG");
            return NULL;
        }
        return pReader;
    }
#ifndef ANDROID
    if (outputChunkType == Chunker::JPEG2000) {
        return encodeImg (pBMPImage, "JPEG2000", &JasperWrapper::convertToJPEG2000);
    }
    if (outputChunkType == Chunker::PNG) {
        return encodeImg (pBMPImage, "PNG", &PNGLibWrapper::convertBMPtoPNG);
    }
#endif
    checkAndLogMsg (pszMethodName, unsupportedType ("output", outputChunkType));
    return NULL;
}

bool ImageCodec::supports (Chunker::Type type)
{
    switch (type) {
        case Chunker::BMP:
        case Chunker::JPEG:
        case Chunker::JPEG2000:
        case Chunker::PNG:
            return true;

        default:
            return false;
    }
}

