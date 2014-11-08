/*
 * MessageSender.cpp
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

#include "MessageSender.h"

#include "Mocket.h"

#include <stdarg.h>


MessageSender::MessageSender (const MessageSender &src)
{
   _pMocket = src._pMocket;
   _bReliable = src._bReliable;
   _bSequenced = src._bSequenced;
   _ui32DefaultEnqueueTimeout = src._ui32DefaultEnqueueTimeout;
   _ui32DefaultRetryTimeout = src._ui32DefaultRetryTimeout;
}

int MessageSender::send (const void *pBuf, uint32 ui32BufSize)
{
    return _pMocket->send (_bReliable, _bSequenced, pBuf, ui32BufSize, 0, DEFAULT_PRIORITY, _ui32DefaultEnqueueTimeout, _ui32DefaultRetryTimeout);
}

int MessageSender::gsend (const void *pBuf1, uint32 ui32BufSize1, ...)
{
    va_list valist1, valist2;
    va_start (valist1, ui32BufSize1);
    va_start (valist2, ui32BufSize1);
    int rc = _pMocket->gsend (_bReliable, _bSequenced, 0, DEFAULT_PRIORITY, _ui32DefaultEnqueueTimeout, _ui32DefaultRetryTimeout, pBuf1, ui32BufSize1, valist1, valist2);
    va_end (valist1);
    va_end (valist2);
    return rc;
}

int MessageSender::send (const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority)
{
    return _pMocket->send (_bReliable, _bSequenced, pBuf, ui32BufSize, ui16Tag, ui8Priority, _ui32DefaultEnqueueTimeout, _ui32DefaultRetryTimeout);
}

int MessageSender::send (const void *pBuf, uint32 ui32BufSize, Params *pParams)
{
    return _pMocket->send (_bReliable, _bSequenced, pBuf, ui32BufSize, pParams->getTag(), pParams->getPriority(), pParams->getEnqueueTimeout(), pParams->getRetryTimeout());
}

int MessageSender::send (const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                         uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    return _pMocket->send (_bReliable, _bSequenced, pBuf, ui32BufSize, ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

int MessageSender::replace (const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag)
{
    return _pMocket->replace (_bReliable, _bSequenced, pBuf, ui32BufSize, ui16OldTag, ui16NewTag, DEFAULT_PRIORITY, _ui32DefaultEnqueueTimeout, _ui32DefaultRetryTimeout);
}

int MessageSender::replace (const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, Params *pParams)
{
    return _pMocket->replace (_bReliable, _bSequenced, pBuf, ui32BufSize, ui16OldTag, pParams->getTag(), pParams->getPriority(), pParams->getEnqueueTimeout(), pParams->getRetryTimeout());
}

int MessageSender::replace (const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
                            uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    return _pMocket->replace (_bReliable, _bSequenced, pBuf, ui32BufSize, ui16OldTag, ui16NewTag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

int MessageSender::cancel (uint16 ui16TagId)
{
    return _pMocket->cancel (_bReliable, _bSequenced, ui16TagId);
}
