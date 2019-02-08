/*
 * SerializationUtil.cpp
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Created on June 16, 2015, 12:25 PM
 */

#include "SerializationUtil.h"

#include "DArray.h"
#include "Writer.h"

using namespace NOMADSUtil;

char ** NOMADSUtil::readStringArray (Reader *pReader, SimpleCommHelper2::Error &error)
{
    if (pReader == NULL) {
        error = SimpleCommHelper2::ProtocolError;
        return NULL;
    }
    if (error !=  SimpleCommHelper2::None) {
        return NULL;
    }
    DArray<char *> outgoingInterfaces;
    int rc = 0;
    char *psz = NULL;
    for (unsigned int i = 0; (rc = pReader->readString (&psz)) >= 0; i++) {
        if (psz == NULL) {
            break;
        }
        outgoingInterfaces[i] = psz;
        psz = NULL;
    }
    if (rc < 0) {
        error = SimpleCommHelper2::CommError;
        return NULL;
    }
    outgoingInterfaces[outgoingInterfaces.getHighestIndex()+1] = NULL;
    return outgoingInterfaces.relinquishData();
}

int NOMADSUtil::writeStringArray (Writer *pWriter, const char **ppszStrings, SimpleCommHelper2::Error &error)
{
    if (pWriter == NULL) {
        error = SimpleCommHelper2::ProtocolError;
        return -1;
    }
    if (error !=  SimpleCommHelper2::None) {
        return -2;
    }
    if (ppszStrings != NULL) {
        for (unsigned int i = 0; ppszStrings[i] != NULL; i++) {
            if (pWriter->writeString (ppszStrings[i]) < 0) {
               error = SimpleCommHelper2::CommError;
               return -3;
            }
        }
    }
    static const char *EMPTY_STRING = "";
    if (pWriter->writeString (EMPTY_STRING) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    return 0;
}

