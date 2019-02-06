/*
 * MocketWriter.cpp
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "MocketWriter.h"
#include "StreamMocket.h"


MocketWriter::MocketWriter (StreamMocket *pMocket, bool bDeleteWhenDone)
{
    _pStreamMocket = pMocket;
    _bDeleteWhenDone = bDeleteWhenDone;
}

MocketWriter::~MocketWriter()
{
    if (_bDeleteWhenDone) {
        delete _pStreamMocket;
    }
    _pStreamMocket = nullptr;
}

int MocketWriter::writeBytes (const void *pBuf, unsigned long ulCount)
{
    return _pStreamMocket->send (pBuf, ulCount);
}

int MocketWriter::flush()
{
    return _pStreamMocket->flush();
}

int MocketWriter::close()
{
    return _pStreamMocket->close();
}
