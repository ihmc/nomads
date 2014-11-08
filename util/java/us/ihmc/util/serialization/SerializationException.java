/*
 * SerializationException.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

package us.ihmc.util.serialization;

/**
 * Manages the exceptions produced during the serialization process
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class SerializationException extends Exception
{
    /**
     * Constructs a <code>SerializationException</code> with no message
     */
    public SerializationException()
    {
    }

    /**
     * Constructs a <code>SerializationException</code> with the given message
     * @param msg description of exception
     */
    public SerializationException (String msg)
    {
        super (msg);
    }

    /**
     * Constructs a <code>SerializationException</code> with the given cause
     * @param e the reason of the exception
     */
    public SerializationException (Throwable e)
    {
        super (e);
    }

    /**
     * Returns the exception message
     * @return the exception message
     */
    public String getMessage()
    {
        return super.getMessage();
    }
}
