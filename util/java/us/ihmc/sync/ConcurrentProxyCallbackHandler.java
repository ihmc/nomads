/*
 * ConcurrentProxyCallbackHandler.java
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

import us.ihmc.util.ManageableThread;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public abstract class ConcurrentProxyCallbackHandler extends ManageableThread
{
    public ConcurrentProxyCallbackHandler (ConcurrentProxy proxy)
    {
        _proxy = proxy;
    }

    /**
     * Call this method as first operation in run() implementation
     */
    protected void setCallbackThreadId()
    {
        _proxy.setCallbackThreadId (getId());
    }

    private final ConcurrentProxy _proxy;
}
