/*
 * CallbackHandler.java
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

package us.ihmc.util.proxy;

import us.ihmc.comm.CommHelper;

/**
 *
 * @author gbenincasa
 */
public abstract class CallbackHandler implements Runnable
{
    protected final Stub _stub;
    protected final CommHelper _commHelper;

    public CallbackHandler (Stub stub, CommHelper commHelper)
    {
        _stub = stub;
        _commHelper = commHelper;
    }

    void closeConnection() {
        try {
            _commHelper.closeConnection();
        }
        catch(Exception e) {}
    }

    public abstract void requestTermination();
}
