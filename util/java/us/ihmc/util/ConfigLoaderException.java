/*
 * ConfigLoaderException.java
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
 * ConfigLoaderException
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.5 $
 * @ $Date: 2016/06/09 20:02:46 $
 * @ Created on Jul 20, 2005 at 2:37:58 PM
 */
public class ConfigLoaderException extends RuntimeException
{

    private static final long serialVersionUID = 8674863088481829943L;

    /**
     * Builds a ConfigLoader Exception (which is a RuntimException)
     * @param e
     */
    public ConfigLoaderException (Exception e)
    {
        super (e);
    }

    /**
     * Builds a ConfigLoader Exception (extends RuntimeException)
     * @param message
     */
    public ConfigLoaderException (String message)
    {
        super (message);
    }
}
