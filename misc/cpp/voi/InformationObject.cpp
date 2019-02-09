/*
 * InformationObject.cpp
 *
 * This file is part of the IHMC Voi Library/Component
 * Copyright (c) 2008-2017 IHMC.
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
 * Created on February 14, 2017
 */

#include "InformationObject.h"

#include "MetadataInterface.h"

using namespace IHMC_VOI;

InformationObject::InformationObject (MetadataInterface *pMetadata, uint32 ui32DataLen)
    : _ui32DataLen (ui32DataLen),
      _pMetadata (pMetadata)
{
}

InformationObject::~InformationObject (void)
{
}

MetadataInterface * InformationObject::getMetadata (void) const
{
    return _pMetadata;
}

//---------------------------------------------------------

MutableInformationObject::MutableInformationObject (MetadataInterface *pMetadata, void *pData, uint32 ui32DataLen)
    : InformationObject (pMetadata, ui32DataLen),
      _pData (pData)
{
}

MutableInformationObject::~MutableInformationObject (void)
{
    if (_pMetadata != NULL) {
        delete _pMetadata;
    }
    if (_pData != NULL) {
        free (_pData);
    }
}

const void * MutableInformationObject::getData (uint32 &ui32DataLen) const
{
    ui32DataLen = _ui32DataLen;
    return _pData;
}

//---------------------------------------------------------

ImmutableInformationObject::ImmutableInformationObject (MetadataInterface *pMetadata, const void *pData, uint32 ui32DataLen)
    : InformationObject (pMetadata, ui32DataLen),
      _pData (pData)
{
}

ImmutableInformationObject::~ImmutableInformationObject (void)
{
}

const void * ImmutableInformationObject::getData (uint32 &ui32DataLen) const
{
    ui32DataLen = _ui32DataLen;
    return _pData;
}

