/*
 * UUID.cpp
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

#include "UUID.h"

#include "UUIDGenerator.h"

#include <memory.h>
#include <string.h>

using namespace NOMADSUtil;

UUID::UUID (void)
{
    memset (auchUUID, 0, sizeof (auchUUID));
    memset (szUUID, 0, sizeof (szUUID));
}

UUID::UUID (const unsigned char *puchUUID)
{
    UUIDGenerator uuidGen;
    memcpy (auchUUID, puchUUID, sizeof (auchUUID));
    strcpy (szUUID, uuidGen.convertToString (puchUUID));
}

UUID::UUID (const UUID &src)
{
    memcpy (auchUUID, src.auchUUID, sizeof(auchUUID));
    memcpy (szUUID, src.szUUID, sizeof(szUUID));
}

int UUID::generate (void)
{
    UUIDGenerator uuidGen;
    int rc = uuidGen.create (auchUUID);
    strcpy (szUUID, uuidGen.convertToString (auchUUID));
    return rc;
}

void UUID::set (const unsigned char *puchUUID)
{
    UUIDGenerator uuidGen;
    memcpy (auchUUID, puchUUID, sizeof (auchUUID));
    strcpy (szUUID, uuidGen.convertToString (puchUUID));
}

bool UUID::operator == (const UUID &rhsUUID)
{
    return (0 == memcmp (auchUUID, rhsUUID.auchUUID, sizeof (auchUUID)));
}

UUID & UUID::operator = (const UUID &src)
{
    memcpy (auchUUID, src.auchUUID, sizeof(auchUUID));
    memcpy (szUUID, src.szUUID, sizeof(szUUID));

    return *this;
}
