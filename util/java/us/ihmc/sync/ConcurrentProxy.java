/*
 * ConcurrentProxy.java
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

package us.ihmc.sync;

import java.util.ConcurrentModificationException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class ConcurrentProxy
{
    void setCallbackThreadId (long callbackTheadId)
    {
        _callbackTheadId = callbackTheadId;
    }

    protected void checkConcurrentModification (String exceptionMsg)
    {
        if (Thread.currentThread().getId() == _callbackTheadId) {
            throw new ConcurrentModificationException (exceptionMsg);
        }
    }

    private long _callbackTheadId;
}
