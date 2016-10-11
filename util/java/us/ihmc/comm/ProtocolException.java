/**
 * ProtocolException.java
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

/**
 * ProtocolException.java
 */
public class ProtocolException extends Exception
{
    /**
     * Constructs a <code>ProtocolException</code> with no message
     */
    public ProtocolException()
    {
    }

    /**
     * Constructs a <code>ProtocolException</code> with the given message
     *
     * @param   msg     description of exception
     */
    public ProtocolException (String msg)
    {
        super (msg);
    }
}