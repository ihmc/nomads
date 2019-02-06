/*
 * StreamServerMocket.cpp
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

#include "StreamServerMocket.h"

#include "ServerMocket.h"
#include "StreamMocket.h"


StreamServerMocket::StreamServerMocket (void)
{
    _pMSMocket = new ServerMocket();
}

StreamServerMocket::~StreamServerMocket (void)
{
    delete _pMSMocket;
    _pMSMocket = nullptr;
}

int StreamServerMocket::listen (uint16 ui16Port)
{
    return _pMSMocket->listen (ui16Port);
}

int StreamServerMocket::listen (uint16 ui16Port, const char *pszListenAddr)
{
    return _pMSMocket->listen (ui16Port, pszListenAddr);
}

StreamMocket * StreamServerMocket::accept (void)
{
    Mocket *pMMocket = _pMSMocket->accept();
    if (pMMocket != nullptr) {
        return new StreamMocket (pMMocket);
    }
    else {
        return nullptr;
    }
}

int StreamServerMocket::close (void)
{
    return _pMSMocket->close();
}
