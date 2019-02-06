/*
 * Dime.cpp
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

#include "Dime.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "EndianHelper.h"
#include "Reader.h"

using namespace NOMADSUtil;

Dime::Dime (uint8 ui8Version)
{
    _ui8Version = ui8Version;
    _ui16RecordsNum = 0;
    _ui32DimeLength = 0;
}

Dime::Dime (void *pDimeMsg, uint32 ui32MsgLength, uint8 ui8Version)
{
    _ui8Version = ui8Version;
    _ui16RecordsNum = 0;
    _ui32DimeLength = 0;
    init (pDimeMsg, ui32MsgLength);
}

Dime::Dime (Reader *pReader)
{
    _ui8Version = DEFAULT_DIME_VERSION;
    _ui16RecordsNum = 0;
    _ui32DimeLength = 0;
    init (pReader);
}

Dime::~Dime (void)
{
    for (int i = 0; i < _ui16RecordsNum; i++) {
        if (_records[i] != NULL) {
            delete _records[i];
        }
    }
}

int Dime::addRecord (const char *pszPayloadType,
                     void *pPayload,
                     uint32 ui32PayloadSize,
                     bool bChunk,
                     const char *pszPayloadIdentifier)
{
    DimeRecord *pdr = new DimeRecord();
    pdr->setPayload (pPayload, ui32PayloadSize);
    pdr->setID (pszPayloadIdentifier);
    pdr->setType (pszPayloadType);
    _records[_ui16RecordsNum++] = pdr;
    _ui32DimeLength += pdr->getRecordLength();

    return 0;
}

int Dime::getDime (void *pDime)
{
    uint32 ui32Index = 0;

    _records[0]->setMessageBeginFlag (true);
    _records[_ui16RecordsNum-1]->setMessageEndFlag (true);
    for (int i = 0; i < _ui16RecordsNum; i++) {
        _records[i]->getRecord (pDime, ui32Index);
        ui32Index += _records[i]->getRecordLength();
    }

    return 0;
}

int Dime::getDime (Writer *pWriter)
{
    _records[0]->setMessageBeginFlag (true);
    _records[_ui16RecordsNum-1]->setMessageEndFlag (true);
    for (int i = 0; i < _ui16RecordsNum; i++) {
        int rc = _records[i]->getRecord (pWriter);
        if (rc) {
            return rc;
        }
    }

    return 0;
}

int Dime::init (void *pDimeMsg, uint32 ui32MsgLength)
{
    // TODO: ui32MsgLength is never used!!!
    unsigned char *psBuf = (unsigned char*) pDimeMsg;
    uint16 ui16Aux, ui16IdLength, ui16TypeLength;
    uint32 ui32Aux, ui32DataLength, ui32Offset = 0;
    char *psBufAux;
    bool bFlag;
    uint32 ui32Index = 0;

    while (true) {
        DimeRecord *pDimeRecord = new DimeRecord();

        // Process the MB, ME, CF flags.
        bFlag = (psBuf[ui32Offset + 0] & 0x80) == 0x80;
        pDimeRecord->setMessageBeginFlag (bFlag);

        bFlag = (psBuf[ui32Offset + 0] & 0x40) == 0x40;
        pDimeRecord->setMessageEndFlag (bFlag);

        bFlag = (psBuf[ui32Offset + 0] & 0x20) == 0x20;
        pDimeRecord->setChunkFlag (bFlag);

        // Get the ID_LENGTH field
        memcpy (&ui16Aux, psBuf + ui32Offset, sizeof (uint16));
        ui16IdLength = EndianHelper::htons (ui16Aux);
        ui16IdLength &= 0x1FFF;

        // Get the TNF field

        // Get the TYPE_LENGTH field
        memcpy (&ui16Aux, psBuf + ui32Offset + 2, sizeof(uint16));
        ui16TypeLength = EndianHelper::htons (ui16Aux);
        ui16TypeLength &= 0x1FFF;

        // Get the DATA_LENGTH field
        memcpy (&ui32Aux, psBuf + ui32Offset + 4, sizeof(uint32));
        ui32DataLength = EndianHelper::htonl (ui32Aux);

/*
printf("Dime::init() mb flag = %s\n", (pDimeRecord->getMessageBeginFlag()) ? "true" : "false");
printf("Dime::init() me flag = %s\n", (pDimeRecord->getMessageEndFlag()) ? "true" : "false");
printf("Dime::init() cb flag = %s\n", (pDimeRecord->getChunkFlag()) ? "true" : "false");

printf("Dime::init() idLength   = %d\n", ui16IdLength);
printf("Dime::init() typeLength = %d\n", ui16TypeLength);
printf("Dime::init() dataLength = %d\n", ui32DataLength);
*/

        // Get the ID field
        if (ui16IdLength > 0) {
            psBufAux = new char[ui16IdLength + 1];
            memset (psBufAux, 0, ui16IdLength + 1);
            memcpy (psBufAux, psBuf + ui32Offset + 8, ui16IdLength);
            pDimeRecord->setID (psBufAux);
        }

        // Get the TYPE field
        uint32 indexAux = (ui16IdLength % 4 == 0) ? ui16IdLength : (ui16IdLength + 4 - (ui16IdLength % 4));
        if (ui16TypeLength > 0) {
            psBufAux = new char[ui16TypeLength + 1];
            memset (psBufAux, 0, ui16TypeLength + 1);
            memcpy (psBufAux, psBuf + ui32Offset + 8 + indexAux, ui16TypeLength);
            pDimeRecord->setType (psBufAux);
        }

        // Get the PAYLOAD
        if (ui32DataLength > 0) {
            indexAux += (ui16TypeLength % 4 == 0) ? ui16TypeLength : (ui16TypeLength + 4 - (ui16TypeLength % 4));
            psBufAux = new char[ui32DataLength];
            memcpy (psBufAux, psBuf + ui32Offset + 8 + indexAux, ui32DataLength);
            pDimeRecord->setPayload (psBufAux, ui32DataLength);
        }

        _records[_ui16RecordsNum++] = pDimeRecord;

        ui32Offset += pDimeRecord->getRecordLength();

        if (pDimeRecord->getMessageEndFlag()) {
            break;
        }
    }

    _ui32DimeLength = ui32Offset;

    return 0;
}

int Dime::init (Reader *pReader)
{
    // TODO: ui32MsgLength is never used!!!
    // unsigned char* psBuf = (unsigned char*) pDimeMsg;
    uint16 ui16Aux, ui16IdLength, ui16TypeLength;
    uint32 ui32Aux, ui32DataLength, ui32Offset = 0;
    const bool bDebug = false;

    uint32 ui32Padding, ui32TotalPadding;

    char *psBufAux;
    bool bFlag;
    bool bErrReading = false;

    char *pHeader = (char*) malloc(8);

    while (true) {
        DimeRecord *pDimeRecord = new DimeRecord();
        if (0 != pReader->readBytes (pHeader, 8)) {
            bErrReading = true;
            break;
        }

        ui32TotalPadding = 0;

        // Process the MB, ME, CF flags.
        bFlag = (pHeader[0] & 0x80) == 0x80;
        pDimeRecord->setMessageBeginFlag (bFlag);
        if (bDebug) printf("Dime:: init MB=%s\n", bFlag? "true" : "false");

        bFlag = (pHeader[0] & 0x40) == 0x40;
        pDimeRecord->setMessageEndFlag (bFlag);
        if (bDebug) printf("Dime:: init ME=%s\n", bFlag? "true" : "false");

        bFlag = (pHeader[0] & 0x20) == 0x20;
        pDimeRecord->setChunkFlag (bFlag);
        if (bDebug) printf("Dime:: init CF=%s\n", bFlag? "true" : "false");

        // Get the ID_LENGTH field
        memcpy (&ui16Aux, pHeader, sizeof (uint16));
        ui16IdLength = EndianHelper::htons (ui16Aux);
        ui16IdLength &= 0x1FFF;
        if (bDebug) printf("Dime:: init ID_LENGTH=%d\n", ui16IdLength);

        // Get the TNF field
        // ...

        // Get the TYPE_LENGTH field
        memcpy (&ui16Aux, pHeader + 2, sizeof(uint16));
        ui16TypeLength = EndianHelper::htons (ui16Aux);
        ui16TypeLength &= 0x1FFF;
        if (bDebug) printf("Dime:: init TYPE_LENGTH=%d\n", ui16TypeLength);

        // Get the DATA_LENGTH field
        memcpy (&ui32Aux, pHeader + 4, sizeof(uint32));
        ui32DataLength = EndianHelper::htonl (ui32Aux);
        if (bDebug) printf("Dime:: init DATA_LENGTH=%d\n", ui32DataLength);

        // Get the ID field
        if (ui16IdLength > 0) {
            psBufAux = new char[ui16IdLength + 1];
            memset (psBufAux, 0, ui16IdLength + 1);
            if (0!= pReader->readBytes(psBufAux, ui16IdLength)) {
                if (bDebug) printf("Dime:: error reading the ID field.\n");
                bErrReading = true;
                break;
            }
            pDimeRecord->setID (psBufAux);
        }

        // Throw away ui32Padding number of bytes  (for skipping padding after the ID field).
        ui32Padding = (4 - (ui16IdLength % 4)) % 4;
        if (ui32Padding > 0) {
            if (0 != pReader->readBytes(pHeader, ui32Padding)) {
                if (bDebug) printf("Dime:: error reading the padding (%d bytes) after the ID field\n", ui32Padding);
                bErrReading = true;
                break;
            }
        }
        ui32TotalPadding += ui32Padding;

        // Get the TYPE field
        if (ui16TypeLength > 0) {
            psBufAux = new char[ui16TypeLength + 1];
            memset (psBufAux, 0, ui16TypeLength + 1);
            if (0 != pReader->readBytes(psBufAux, ui16TypeLength)) {
                if (bDebug) printf("Dime:: error reading the TYPE field.\n");
                bErrReading = true;
                break;
            }
            pDimeRecord->setType (psBufAux);
        }

        // Throw away ui32Padding number of bytes  (for skipping padding after the TYPE field).
        ui32Padding = (4 - (ui16TypeLength % 4)) % 4;
        if (ui32Padding > 0) {
            if (0 != pReader->readBytes(pHeader, ui32Padding)) {
                if (bDebug) printf("Dime:: error reading the padding (%d bytes) after the TYPE field\n", ui32Padding);
                bErrReading = true;
                break;
            }
        }
        ui32TotalPadding += ui32Padding;

        // Get the PAYLOAD
        if (ui32DataLength > 0) {
            psBufAux = new char[ui32DataLength];
            if (0 != pReader->readBytes(psBufAux, ui32DataLength)) {
                if (bDebug) printf("Dime:: error reading the PAYLOAD.\n");
                bErrReading = true;
                break;
            }
            pDimeRecord->setPayload (psBufAux, ui32DataLength);
        }

        // Throw away ui32Padding number of bytes  (for skipping padding after the PAYLOAD).
        ui32Padding = (4 - (ui32DataLength % 4)) % 4;
        if (ui32Padding > 0) {
            if (0 != pReader->readBytes(pHeader, ui32Padding)) {
                if (bDebug) printf("Dime:: error reading the padding (%d bytes) after the PAYLOAD field\n", ui32Padding);
                bErrReading = true;
                break;
            }
        }
        ui32TotalPadding += ui32Padding;

        _records[_ui16RecordsNum++] = pDimeRecord;

        _ui32DimeLength += 8; //record header size.
        _ui32DimeLength += ui16TypeLength + ui16IdLength + ui32DataLength;
        _ui32DimeLength += ui32TotalPadding;

        if (pDimeRecord->getMessageEndFlag()) {
            break;
        }
    } // while

    free (pHeader);

    if (bErrReading) {
        if (bDebug) printf("error reading the dime record. stop the parsing.\n");

        // If there was an error...
        // clean up
        for (int i = 0; i < _ui16RecordsNum; i++) {
            if (_records[i] != NULL) {
                delete _records[i];
            }
        }

        // and fail
        return -1;
    }

    return 0;
} //init()

void Dime::print()
{
    printf ("::: Start of DIME Message :::\n");
    printf ("::: Total Dime length %d :::\n", getLength());

    for (int i = 0; i < _ui16RecordsNum; i++) {
        DimeRecord* dr = _records[i];
        dr->print();
    }

    printf ("::: End of DIME Message :::\n");
}

// //////////////////////////////////////////////////////////////////
// DimeRecord
// //////////////////////////////////////////////////////////////////
DimeRecord::DimeRecord (void)
{
    _bMessageBeginFlag = false;
    _bMessageEndFlag = false;
    _bChunkFlag = false;
    _pPayload = NULL;
}

DimeRecord::~DimeRecord (void)
{
    if (_pPayload != NULL) {
        free (_pPayload);
    }
}

void *DimeRecord::getRecord (void)
{
    uint32 totalSize = getRecordLength();
    void *pRecord = malloc (totalSize);
    getRecord (pRecord, 0);
    return pRecord;
}

void *DimeRecord::getRecord (void *pBuf, uint32 ui32Offset)
{
    uint32 totalSize = getRecordLength();
    uint32 index;
    uint16 ui16Aux;
    uint32 ui32Aux;

    char *pRecord = (char*) pBuf + ui32Offset;

    memset (pRecord, 0, totalSize);

    if (_id != NULL) {
        ui16Aux = EndianHelper::htons (_id.length());
        memcpy (pRecord, &ui16Aux, sizeof(uint16));
    }

    pRecord[0] &= 0x1F;
    if (_bMessageBeginFlag) {
        pRecord[0] |= 0x80;
    }
    if (_bMessageEndFlag) {
        pRecord[0] |= 0x40;
    }
    if (_bChunkFlag) {
        pRecord[0] |= 0x20;
    }

    if (_type != NULL) {
        ui16Aux = EndianHelper::htons (_type.length());
        memcpy (pRecord + 2, &ui16Aux, sizeof(uint16));
    }

    pRecord[2] &= 0x1F;
    // Set the proper TNF value here.

    ui32Aux = EndianHelper::htonl (_ui32PayloadLength);
    memcpy (pRecord + 4, &ui32Aux, sizeof(uint32));

    // pRecord[8]: ID + Padding
    if (_id != NULL) {
        memcpy (pRecord + 8, (char*)_id, _id.length());

        index = 8 + (_id.length() % 4 == 0 ? _id.length()
                                           : _id.length() + 4 - (_id.length() % 4) );
    }
    else {
        index = 8;
    }

    // pRecord[index]: Type + Padding
    if (_type != NULL) {
        memcpy (pRecord + index, (char*) _type, _type.length());
        index = index + (_type.length() % 4 == 0 ? _type.length()
                                                 : _type.length() + 4 - (_type.length() % 4) );
    }
    else {
        printf ("WARNING:: TYPE field seems to be NULL.");
    }

    // Copy the data
    memcpy (pRecord + index, _pPayload, _ui32PayloadLength);

    return pRecord;
}

int DimeRecord::getRecord (Writer *pWriter)
{
    char header[8];
    char padding[3];

    memset (header, 0, 8);
    memset (padding, 0, 3);

    uint16 ui16Aux;
    uint32 ui32Aux;

    uint8  ui8Padding;

    if (_id != NULL) {
        ui16Aux = EndianHelper::htons (_id.length());
        memcpy (header, &ui16Aux, sizeof(uint16));
    }

    //first, prepare the header.
    header[0] &= 0x1F;
    if (_bMessageBeginFlag) {
        header[0] |= 0x80;
    }
    if (_bMessageEndFlag) {
        header[0] |= 0x40;
    }
    if (_bChunkFlag) {
        header[0] |= 0x20;
    }

    if (_type != NULL) {
        ui16Aux = EndianHelper::htons (_type.length());
        memcpy (header + 2, &ui16Aux, sizeof(uint16));
    }

    header[2] &= 0x1F;
    // Set the proper TNF value here.

    ui32Aux = EndianHelper::htonl (_ui32PayloadLength);
    memcpy (header + 4, &ui32Aux, sizeof(uint32));

    // write the header.
    if (pWriter->writeBytes(header, 8)) {
        return -1;
    }

    // write ID + padding.
    if (_id.length() > 0) {
        if ( pWriter->writeBytes((char*)_id, _id.length()) ) {
            return -2;
        }
        if ( (ui8Padding = (4 - (_id.length() % 4)) % 4) != 0 ) {
            if ( pWriter->writeBytes(padding, ui8Padding) ) {
                return -3;
            }
        }
    }

    // write Type + Padding
    if (_type != NULL) {
        if ( pWriter->writeBytes((char*)_type, _type.length()) ) {
            return -4;
        }
        if ( (ui8Padding = (4 - (_type.length() % 4)) % 4) != 0 ) {
            if ( pWriter->writeBytes(padding, ui8Padding) ) {
                return -5;
            }
        }
    }
    else {
        printf ("Dime.cpp:: WARNING:: TYPE field seems to be NULL.");
    }

    // Write the Record payload data and padding.
    if (_ui32PayloadLength > 0) {
        if ( pWriter->writeBytes((char*)_pPayload, _ui32PayloadLength) ) {
            return -6;
        }
        if ( (ui8Padding = (4 - (_ui32PayloadLength % 4)) % 4) != 0 ) {
            if ( pWriter->writeBytes(padding, ui8Padding) ) {
                return -7;
            }
        }
    }

    return 0;
}

uint32 DimeRecord::getRecordLength (void)
{
    uint32 ui32RecLength = 8; // The header size
    uint32 ui32Aux;

    // Add the length of the 'type' field (with padding)
    if (_type != NULL) {
        ui32Aux = _type.length();
        ui32RecLength += (ui32Aux % 4 == 0) ? ui32Aux
                                            : ( ui32Aux + 4 - (ui32Aux % 4) ); //align to 32 bits.
    }

    // Add the length of the 'ID' field (with padding)
    if (_id != NULL) {
        ui32Aux = _id.length();
        ui32RecLength += (ui32Aux % 4 == 0) ? ui32Aux
                                            : ( ui32Aux + 4 - (ui32Aux % 4) ); //align to 32 bits.
    }

    // Now, add the size of the payload (with padding)
    ui32Aux = _ui32PayloadLength;
    ui32RecLength += (ui32Aux % 4 == 0) ? ui32Aux
                                        : ( ui32Aux + 4 - (ui32Aux % 4) ); //align to 32 bits.

    return ui32RecLength;
}

void DimeRecord::print()
{
    printf ("::: Dime Record :::\n");
    printf ("\tMB = %s\n",  (_bMessageBeginFlag ? "true" : "false"));
    printf ("\tME = %s\n", (_bMessageEndFlag ? "true" : "false"));
    printf ("\tCF = %s\n", (_bChunkFlag ? "true" : "false"));
    printf ("\tID = %s\n", (const char*)_id);
    printf ("\tType = %s\n", (const char*) _type);
    printf ("\tPayloadLength = %d\n",  _ui32PayloadLength);
    printf ("\tPayload ::::::: [");
    if (_pPayload != NULL) {
        int lim = 70;
        lim = (_ui32PayloadLength < (uint32) lim) ? _ui32PayloadLength : lim;
        for (int i = 0; i < lim; i++) {
            char c = ((char*)_pPayload)[i];
            printf("%c", c);
        }
    }
    printf ("]\n");
}
