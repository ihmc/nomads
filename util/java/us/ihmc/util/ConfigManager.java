/*
 * ConfigManager.java
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

package us.ihmc.util;

import java.io.FileInputStream;
import java.io.IOException;

import java.util.Properties;

/**
 * ConfigManager.java
 *
 * Created on July 25, 2007, 7:04 PM
 *
 * @author nsuri
 */
public class ConfigManager
{
    public ConfigManager (String pathToConfigFile)
        throws IOException
    {
        _properties = new Properties();
        _properties.load (new FileInputStream (pathToConfigFile));
    }

    public boolean hasValue (String key)
    {
        return _properties.containsKey (key);
    }
        
    public void setValue (String key, String value)
    {
        _properties.setProperty (key, value);
    }

    public void setValue (String key, int value)
    {
        _properties.setProperty (key, Integer.toString (value));
    }

    public String getValue (String key)
    {
        return _properties.getProperty (key);
    }

    public int getValueAsInt (String key)
    {
        return Integer.parseInt (_properties.getProperty (key));
    }

    public boolean getValueAsBool (String key)
    {
        String value = _properties.getProperty (key);
        if ((value != null) && (value.equalsIgnoreCase ("true"))) {
            return true;
        }
        else {
            return false;
        }
    }

    public String removeValue (String key)
    {
        return (String) _properties.remove (key);
    }

    private Properties _properties;
}
