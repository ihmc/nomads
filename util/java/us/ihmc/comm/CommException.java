/**
 * CommException.java
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

package us.ihmc.comm;

public class CommException extends Exception
{
    /**
     * Constructs a <code>CommException</code> with no message
     */    
    public CommException()
    {
    }
    
    /**
     * Constructs a <code>CommException</code> with the given message
     * 
     * @param   msg     description of exception
     */
    public CommException (String msg)
    {
        super(msg);
    }

    /**
     * Constructs a <code>CommException</code> with the given message
     *
     * @param  msg the detail message (which is saved for later retrieval
     *         by the {@link #getMessage()} method).
     * @param  cause the cause (which is saved for later retrieval by the
     *         {@link #getCause()} method).
     */
    public CommException (String msg, Throwable cause)
    {
        super(msg, cause);
    }

    /**
     * Constructs a <code>CommException</code> with the given cause
     *
     * @param e the reason of the exception
     */
    public CommException (Throwable e)
    {
        super(e);
    }
}
