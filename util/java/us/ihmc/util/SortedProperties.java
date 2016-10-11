/*
 * SortedProperties.java
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

import java.io.IOException;
import java.io.StringReader;
import java.util.Collections;
import java.util.Enumeration;
import java.util.LinkedHashSet;
import java.util.Properties;

/**
 * SortedProperties.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class SortedProperties extends Properties
{

	private static final long serialVersionUID = -8336236126006306661L;

    /**
     * Returns all the keys
     * @return the keys in the <code>SortedProperties</code>
     */
    public Enumeration<Object> keys ()
    {
        return Collections.<Object>enumeration(_keys);
    }

    /**
     * Sets the property value if value is not null
     * @param key the property key to look for
     * @param value the value to assign to the property
     * @return the value just added if it's not null, or null otherwise
     */
    public synchronized Object setPropertySafely (String key, String value)
    {
        if (value != null) {
            return put(key, value);
        }
        else {
            return null;
        }
    }

    @Override
    public Object put (Object key, Object value)
    {
        _keys.add (key);
        return super.put (key, value);
    }

    /**
     * Loads properties form a <code>String</code>
     * @param propertiesString <code>String</code> containing the properties
     * @throws java.io.IOException
     */
    public void load (String propertiesString) throws IOException
    {
        super.load (new StringReader (propertiesString));
    }

    /**
     * Copies the <code>String</code> properties of the <code>SortedProperties</code> passed as parameter into a new
     * <code>SortedProperties</code> instance
     * @param sp <code>SortedProperties</code> to copy
     * @return the new <code>SortedProperties</code> instance containing the copy of the not null <code>String</code>
     * properties
     */
    public static SortedProperties copyStringProperties (SortedProperties sp)
    {
        SortedProperties copy = new SortedProperties();
        for (String key : sp.stringPropertyNames()) {
            copy.setPropertySafely (key, sp.getProperty (key));
        }

        return copy;
    }

    private final LinkedHashSet<Object> _keys = new LinkedHashSet<Object>();
}

