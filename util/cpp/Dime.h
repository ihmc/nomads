/*
 * Dime.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 */

#ifndef INCL_DIME_H
#define INCL_DIME_H

#include "FTypes.h"
#include "StrClass.h"
#include "DArray.h"
#include "Reader.h"
#include "Writer.h"

/*
 * Direct Internet Message Encapsulation (DIME) is a lightweight, binary message format 
 * that can be used to encapsulate one or more application-defined payloads of arbitrary 
 * type and size into a single message construct.
 *
 * Each payload is described by a type, a length, and an optional identifier. Both URIs 
 * and MIME media type constructs are supported as type identifiers. 
 *
 * The payload length is an integer indicating the number of octets of the payload. The
 * optional payload identifier is a URI enabling cross-referencing between payloads. 
 *
 * DIME payloads may include nested DIME messages or chains of linked chunks of unknown 
 * length at the time the data is generated. 
 *
 * DIME is strictly a message format: it provides no concept of a connection or of a 
 * logical circuit, nor does it address head-of-line problems.
 */

#define DEFAULT_DIME_VERSION    0x01

namespace NOMADSUtil
{

    class DimeRecord;

    class Dime
    {
        public:
            Dime (uint8 ui8Version = DEFAULT_DIME_VERSION);
            Dime (void *pDimeMsg, uint32 pMsgLength, uint8 ui8Version = DEFAULT_DIME_VERSION);
            Dime (Reader *pReader);
            ~Dime (void);

        public:
            int init (void *pDimeMsg, uint32 pMsgLength);
            int init (Reader *pReader);

            //TODO: implement a private method for checking the integrity of a DIME message. 
            //This method will be invoked from the contructor and the init() for checking
            //the DIME integrity before processing it.
            
            /*
             * A DIME record contains a payload described by a type, a 
             * length, and an optional identifier.
             *
             * The usage of the payload types is entirely at the discretion 
             * of the user application. 
             */
            int addRecord (const char *pszPayloadType, 
                           void *pPayload,
                           uint32 ui32PayloadSize,
                           bool bChunk = false,
                           const char *pszPayloadIdentifier = NULL);
            int getDime (void *pDime);
            
            int getDime (Writer *pWriter);

            uint16 getRecordsNum (void);
            uint32 getLength (void);
            uint8 getVersion (void);

            DimeRecord * getRecord (uint16 ui16Idx);

            void print();

        private:
            uint8 _ui8Version;
            uint32 _ui32DimeLength;
            DArray<DimeRecord*> _records;
            uint16 _ui16RecordsNum;
    }; //class Dime

    class DimeRecord
    {
        public:
            DimeRecord(void);
            ~DimeRecord(void);

            void setMessageBeginFlag (bool value);
            void setMessageEndFlag (bool value); 
            void setChunkFlag (bool value);

            bool getMessageBeginFlag (void);
            bool getMessageEndFlag (void);
            bool getChunkFlag (void);

            void setPayload (void *pPayload, uint32 ui32PayloadLength);
            void setType (const char *pszType);
            void setID (const char *pszID);

            void *getPayload (void);
            String getType (void);
            String getID (void);

            uint32 getPayloadLength (void);
            uint32 getRecordLength (void);

            void *getRecord (void);
            void *getRecord (void *pBuff, uint32 ui32Offset);
            int getRecord (Writer *pWriter);

            void print();

        private:
            bool _bMessageBeginFlag;
            bool _bMessageEndFlag;
            bool _bChunkFlag;

            uint8  _ui8TNF;
            uint32 _ui32PayloadLength;

            void *_pPayload;
            String _type;
            String _id;
    }; //class DimeRecord


    ////////////////////////////////////////////////////////////////////////////
    // Dime 
    ////////////////////////////////////////////////////////////////////////////

    inline uint8 Dime::getVersion (void)
    {
        return _ui8Version;
    }

    inline uint32 Dime::getLength (void) 
    {
        return _ui32DimeLength;
    }

    inline uint16 Dime::getRecordsNum (void) 
    {
        return _ui16RecordsNum;
    }


    ////////////////////////////////////////////////////////////////////////////
    // DimeRecord 
    ////////////////////////////////////////////////////////////////////////////

    inline DimeRecord * Dime::getRecord (uint16 ui16Idx)
    {
        return _records[ui16Idx];
    }

    inline void DimeRecord::setMessageBeginFlag (bool value)
    {
        _bMessageBeginFlag = value;
    }

    inline void DimeRecord::setMessageEndFlag (bool value)
    {
        _bMessageEndFlag = value;
    }

    inline void DimeRecord::setChunkFlag (bool value)
    {
        _bChunkFlag = value;
    }

    inline bool DimeRecord::getMessageBeginFlag (void) 
    {
        return _bMessageBeginFlag;
    }

    inline bool DimeRecord::getMessageEndFlag (void) 
    {
        return _bMessageEndFlag;
    }

    inline bool DimeRecord::getChunkFlag (void) 
    {
        return _bChunkFlag;
    }

    inline void DimeRecord::setType (const char *pszType)
    {
        _type = pszType;
    }

    inline void DimeRecord::setID (const char *pszID)
    {
        _id = pszID;
    }

    inline String DimeRecord::getType (void) 
    {
        return _type;
    }

    inline String DimeRecord::getID (void) 
    {
        return _id;
    }

    inline void DimeRecord::setPayload (void *pPayload, uint32 ui32PayloadLength)
    {
        _pPayload = pPayload;
        _ui32PayloadLength = ui32PayloadLength;
    }

    inline void* DimeRecord::getPayload (void) 
    {
        return _pPayload;
    }

    inline uint32 DimeRecord::getPayloadLength (void) 
    {
        return _ui32PayloadLength;
    }

}

#endif   // #ifndef INCL_DIME_H
