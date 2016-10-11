/*
 * ConfigLoader.java
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

import java.util.Properties;
import java.util.Enumeration;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

/**
 * ConfigLoader
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.23 $
 *          Created on Aug 4, 2004 at 8:29:01 AM
 *          $Date: 2016/06/09 20:02:46 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */

public class ConfigLoader
{
    /**
     * Initializes the properties in this instance using the properties file
     * specified.
     * This version of the constructor does not initialize the home directory.
     * 
     * @param pathToFile the path to the config file that should be loaded.
     */
    public ConfigLoader (String pathToFile)
    {
        try {
            _homeDirectory = null;
            if (File.separator.equals ("/")) {
                pathToFile = pathToFile.replace ('\\','/');
            }
            else {
                pathToFile = pathToFile.replace ('/','\\');
            }
            _properties = loadProperties(new Properties(), pathToFile);
        }
        catch (Exception e) {
            _properties = new Properties();
            System.out.println ("[ConfigLoader Warning] " + e.getMessage());
        }
    }

    /**
     * Builds a local copy of the ConfigLoader. If called directly through the constructor,
     * the ConfigLoader remains un-initialized, with a null 'defaultConfigLoader'.
     *
     * @param homeDirectory the home directory (base-dir) for the application (usually, nomads.home).
     * @param relativePathToFile the relative path to the config file or to the directory from where the ConfigLoader
     *        will automatically load all files terminated in '.cfg' and '.properties'.
     */
    public ConfigLoader (String homeDirectory, String relativePathToFile)
    {
        try {
            _homeDirectory = homeDirectory;
            if (File.separator.equals ("/")) {
                homeDirectory = homeDirectory.replace ('\\','/');
                relativePathToFile = relativePathToFile.replace ('\\','/');
            }
            else {
                homeDirectory = homeDirectory.replace ('/','\\');
                relativePathToFile = relativePathToFile.replace ('/','\\');
            }
            if (!homeDirectory.endsWith (File.separator)) {
                homeDirectory = homeDirectory + File.separator;
            }
            String fullPathToFile = homeDirectory + relativePathToFile;
            //System.out.println ("[ConfigLoader] fullPathToFile: " + fullPathToFile);
            _properties = loadProperties(new Properties(), fullPathToFile);
            //System.out.println ("[ConfigLoader] _properties: " + _properties);
        }
        catch (Exception e) {
            _properties = new Properties();
            System.out.println ("[ConfigLoader Warning] " + e.getMessage());
        }
    }
    
    /**
     * Initializes the default ConfigLoader.
     * 
     * @param pathToFile the path to the config file that should be loaded.
     */
    public static ConfigLoader initDefaultConfigLoader (String pathToFile)
    {
        _configLoader = new ConfigLoader (pathToFile);
        return _configLoader;
    }

    /**
     * Initializes the default ConfigLoader.
     *
     * @param homeDirectory the home directory (base-dir) for the application (usually, nomads.home).
     * @param pathToFile the relative path to the config file or to the directory from where the ConfigLoader
     *        will automatically load all files terminated in '.cfg' and '.properties'.
     */
    public static ConfigLoader initDefaultConfigLoader (String homeDirectory, String pathToFile)
    {
        _configLoader = new ConfigLoader(homeDirectory, pathToFile);
        return _configLoader;
    }

    /**
     * Returns a handle to the default instance to the config-loader that must have been
     * previously configured by the 'initDefaultConfigLoader' method.
     *
     * @return handle to the default ConfigLoader
     * @throws RuntimeException if the configLoader is not initialized.
     */
    public static ConfigLoader getDefaultConfigLoader ()
            throws RuntimeException
    {
        if (_configLoader == null) {
            throw new ConfigLoaderException ("ConfigLoader not initialized");
        }
        return _configLoader;
    }

    //---------------------------------------------------------------------
    public Properties getProperties()
        throws Exception
    {
        return _properties;
    }

    public String getHomeDirectory()
    {
        return _homeDirectory;
    }

    /**
     * Allows a property:value to be set tot
     * @param propertyName
     * @param propertyValue
     */
    public void setProperty (String propertyName, String propertyValue)
    {
        if (propertyName != null && propertyValue != null) {
            _properties.put(propertyName, propertyValue);
        }
    }

    public void removeProperty (String propertyName)
    {
        _properties.remove(propertyName);
    }

    public boolean hasProperty (String sPropName)
    {
        String sprop = _properties.getProperty (sPropName);
        if (sprop == null) {
            return false;
        }
        return true;
    }

    public boolean hasNonEmptyProperty (String sPropName)
    {
        String sprop = _properties.getProperty (sPropName);
        if (sprop == null) {
            return false;
        }
        return (sprop.length() > 0);
    }

    public boolean getPropertyAsBoolean (String sPropName) throws Exception
    {
        String sprop = _properties.getProperty (sPropName);
        if (sprop == null) {
            throw new Exception ("Property (" + sPropName + ") not found");
        }
        return Boolean.valueOf(sprop).booleanValue();
    }

    public boolean getPropertyAsBoolean (String sPropName, boolean defaultValue)
    {
        boolean value = defaultValue;
        try {
            String sprop = _properties.getProperty (sPropName);
            value = Boolean.valueOf(sprop).booleanValue();
        }
        catch (Exception e) {
            return (defaultValue);
        }
        return value;
    }

    //---------------------------------------------------------------------
    public short getPropertyAsShort (String sPropName) throws Exception
    {
        String sprop = _properties.getProperty (sPropName);
        if (sprop == null) {
            throw new Exception ("Property (" + sPropName + ") not found");
        }
        return (Short.parseShort (sprop));
    }

    public short getPropertyAsShort (String sPropName, short defaultValue)
    {
        short ivalue = defaultValue;
        try {
            String sprop = _properties.getProperty (sPropName);
            ivalue = Short.parseShort (sprop);
        }
        catch (Exception e) {
            return (defaultValue);
        }
        return (ivalue);
    }

    //---------------------------------------------------------------------
    public int getPropertyAsInt (String sPropName) throws Exception
    {
        String sprop = _properties.getProperty (sPropName);
        if (sprop == null) {
            throw new Exception ("Property (" + sPropName + ") not found");
        }
        return (Integer.parseInt (sprop));
    }

    public int getPropertyAsInt (String sPropName, int defaultValue)
    {
        int ivalue = defaultValue;
        try {
            String sprop = _properties.getProperty (sPropName);
            ivalue = Integer.parseInt (sprop);
        }
        catch (Exception e) {
            return (defaultValue);
        }
        return (ivalue);
    }

    //---------------------------------------------------------------------
    public long getPropertyAsLong (String sPropName) throws Exception
    {
        String sprop = _properties.getProperty (sPropName);
        if (sprop == null) {
            throw new Exception ("Property (" + sPropName + ") not found");
        }
        return (Long.parseLong(sprop));
    }

    public long getPropertyAsLong (String sPropName, long defaultValue)
    {
        long ivalue = defaultValue;
        try {
            String sprop = _properties.getProperty (sPropName);
            ivalue = Long.parseLong (sprop);
        }
        catch (Exception e) {
            return (defaultValue);
        }
        return (ivalue);
    }

    //---------------------------------------------------------------------
    public double getPropertyAsDouble (String sPropName) throws Exception
    {
        String sprop = _properties.getProperty (sPropName);
        if (sprop == null) {
            throw new Exception ("Property (" + sPropName + ") not found");
        }
        return (Double.parseDouble(sprop));
    }

    public double getPropertyAsDouble (String sPropName, double defaultValue)
    {
        double dvalue = defaultValue;
        try {
            String sprop = _properties.getProperty (sPropName);
            dvalue = Double.parseDouble (sprop);
        }
        catch (Exception e) {
            return (defaultValue);
        }
        return (dvalue);
    }

    //---------------------------------------------------------------------
    public String getProperty (String sPropName)
    {
        return (_properties.getProperty (sPropName));
    }

    public String getProperty (String sPropName, String defaultValue)
    {
        return (_properties.getProperty (sPropName, defaultValue));
    }

    //---------------------------------------------------------------------
    //////////////////////// Private Methods ///////////////////////////////
    private Properties loadProperties(Properties properties, String pathToFile)
            throws IOException
    {
        File file = new File (pathToFile);
        if (file.isDirectory()) {
            File[] flist = file.listFiles();
            for (int i=0; i<flist.length; i++) {
                loadProperties (properties, flist[i].getAbsolutePath());
            }
        }
        else {
            String fileName = file.getName().toLowerCase();
            if (fileName.endsWith(".cfg") || fileName.endsWith(".properties")) {
                Properties props = new Properties();
                props.load(new FileInputStream(file));
                appendProperties (properties, props, file.getAbsolutePath());
            }
        }
        return properties;
    }

    @SuppressWarnings("rawtypes")
    private void appendProperties (Properties properties, Properties props, String pathToFile)
    {
        Enumeration en = props.keys();
        while (en.hasMoreElements())
        {
            String key = (String) en.nextElement();
            if (properties.containsKey(key)) {
                if (_duplicatePropertyWarning) {
                    System.out.println("[ConfigLoader Warning] Ignoring duplicate property (" + key + ") in " + pathToFile);
                }
            }
            else {
                properties.put(key, (String) props.get(key));
            }
        }
    }

    //---------------------------------------------------------------------
    @SuppressWarnings("rawtypes")
    public void showProperties()
    {
        Enumeration en = _properties.keys();
        while (en.hasMoreElements())
        {
            String key = (String) en.nextElement();
            System.out.println(key + "\t:\t" + (String) _properties.getProperty(key));
        }
    }

    private boolean _duplicatePropertyWarning = true;   //system.out warnings about duplicate properties during init.
    private Properties _properties;
    private String _homeDirectory;
    private static ConfigLoader _configLoader;
}
