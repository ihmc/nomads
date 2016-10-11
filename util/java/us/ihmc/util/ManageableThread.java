/*
 * ManageableThread.java
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

package us.ihmc.util;

/**
 * ManageableThread.java
 *
 * Created on September 15, 2007, 11:01 AM
 */
public class ManageableThread extends Thread
{
    public boolean isRunning()
    {
        return isAlive();
    }

    public void requestTermination()
    {
        _terminate = true;
        try {
            if (_termReqListener != null) {
                _termReqListener.terminate();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void requestTerminationAndWait()
    {
        requestTermination();
        try {
            join();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public boolean hasTerminated()
    {
        return !isAlive();
    }
    
    public Exception getTerminatingException()
    {
        return _terminatingException;
    }

    public static interface TerminationRequestListener
    {
        public void terminate();
    }

    protected boolean terminationRequested()
    {
        return _terminate;
    }
    
    protected void setTerminationRequestListener (TerminationRequestListener listener)
    {
        _termReqListener = listener;
    }

    protected void setTerminatingException (Exception e)
    {
        _terminatingException = e;
    }

    private boolean _terminate = false;
    private TerminationRequestListener _termReqListener = null;
    private Exception _terminatingException = null;
}
