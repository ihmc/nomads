/* 
 * StubCallbackHandler.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 27, 2015, 2:58 PM
 */

#include "StubCallbackHandler.h"

#include "Stub.h"

#include "NLFLib.h"
#include "SimpleCommHelper2.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

StubCallbackHandler::StubCallbackHandler (Stub *pStub, SimpleCommHelper2 *pCommHelper, StubUnmarshalFnPtr pUnmarshaller)
    : _pUnmarshaller (pUnmarshaller),
      _pStub (pStub),
      _pCommHelper (pCommHelper)
{
}

StubCallbackHandler::~StubCallbackHandler (void)
{
}

void StubCallbackHandler::run (void)
{
    const char * const pszMethodName = "StubCallbackHandler::run";

    started();
    char tmpbuf[80];
    sprintf (tmpbuf, "StubCallbackHandler %d %p", _pStub->getApplicationId(), _pStub);
    Thread::setName (tmpbuf);
    while (!terminationRequested()) {

        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        const char **apszTokens = _pCommHelper->receiveParsed (error);
        if (terminationRequested()) {
            break;
        }

        if (error == SimpleCommHelper2::None) {
            if (apszTokens != NULL) {
                String methodName (apszTokens[0]);
                methodName.trim();
                const int iIdx = methodName.indexOf (' ');
                if (iIdx > 0) {
                    methodName = methodName.substring (0, methodName.length());
                }
                if (!_pUnmarshaller (_pStub->getApplicationId(), methodName, _pStub, _pCommHelper)) {
                    break;
                }
            }
        }
        else if (error == SimpleCommHelper2::CommError) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed with comm exception.\n");
            if (_pStub->startReconnect()) {
                break;
            }
            sleepForMilliseconds(1000);
        }
        else if (error == SimpleCommHelper2::ProtocolError) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed with protocol exception.\n");
        }
        else {
            // this case should never happen!
            assert (false);
        }
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "id %d terminating\n",
                    _pStub->getApplicationId());
    terminating();
}

void StubCallbackHandler::requestTermination (void)
{
    if (_pCommHelper != NULL) {
        SimpleCommHelper2::Error error;
        _pCommHelper->closeConnection (error);
    }
    ManageableThread::requestTermination();
}

void StubCallbackHandler::requestTerminationAndWait (void)
{
    if (_pCommHelper != NULL) {
        SimpleCommHelper2::Error error;
        _pCommHelper->closeConnection (error);
    }
    ManageableThread::requestTerminationAndWait();
}

