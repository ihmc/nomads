/*
 * Config.java
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

import java.io.FileInputStream;
import java.io.IOException;
import java.util.Properties;
import java.util.logging.Logger;

/**
 * Config.java
 * <p/>
 * Class that helps to manage a software configuration. It's designed to read configuration settings from a
 * .property file and save these information inside a <code>Property</code> object.
 * Extra methods for overwriting keys and querying with strong types are also provided.
 * This class is a singleton.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public final class Config
{
    /**
     * Constructor for <code>Config</code>
     */
    private Config ()
    {
        _properties = new Properties();
    }

    /**
     * Returns the instance of this object
     * @return the <code>Config</code> object instance
     */
    public static Config getInstance ()
    {
        if (INSTANCE == null) {
            INSTANCE = new Config();
        }
        return INSTANCE;
    }

    /**
     * Returns the current internal <code>Properties</code> object.
     *
     * @return a copy of the <code>Properties</code> instance of this Config
     */
    public static Properties getProperties ()
    {
        return getInstance()._properties;
    }

    /**
     * Initialize the <code>Properties</code> instance with values passed by argument.
     *
     * @param values the <code>Properties</code> instance that must be assigned to the current properties.
     */
    public static synchronized void loadConfig (Properties values)
    {
        getInstance()._properties.putAll(values);
    }


    /**
     * Loads settings from a specified .properties configuration file.
     *
     * @param configPath the path of file containing the configuration
     */
    public static synchronized void loadConfig (String configPath)
    {
        CheckUtil.checkNotNull(configPath, "Configuration file path is null");

        try {
            getInstance()._properties.load(new FileInputStream(configPath));
            log.info(String.format("Loading configuration settings found in: %s", configPath));
        }
        catch (IOException e) {
            e.printStackTrace();
            log.severe(String.format("Unable to read configuration from file: %s", configPath));
        }
    }

    /**
     * Return as a string the value associated with a specified key.
     *
     * @param key the key for the desired value.
     * @return the value associated with the key, or null if the key does not exist.
     */
    public static synchronized String getStringValue (String key)
    {
        return getInstance()._properties.getProperty(key);
    }

    /**
     * Return as a string the value associated with a specified key if it's not null, otherwise
     * return the default value.
     *
     * @param key the key for the desired value.
     * @param defaultValue the value to return if the key does not exist.
     * @return the value associated with the key, or defaultValue if the key does not exist.
     */
    public static synchronized String getStringValue (String key, String defaultValue)
    {
        String value = getStringValue (key);
        return value != null ? value : defaultValue;
    }


    /**
     * Return as a Boolean the value associated with a specified key.
     * <p/>
     * Valid values for true are '1' or anything that starts with 't' or 'T'. ie. 'true', 'True', 't' Valid values for
     * false are '0' or anything that starts with 'f' or 'F'. ie. 'false', 'False', 'f'
     *
     * @param key the key for the desired value.
     * @return the value associated with the key, or null if the key does not exist or is not a Boolean or string
     *         representation of an Boolean.
     */
    public static synchronized Boolean getBooleanValue (String key)
    {
        String v = getStringValue(key);
        if (v == null)
            return null;

        if (v.trim().toUpperCase().startsWith("T") || v.trim().equals("1")) {
            return true;
        }
        else if (v.trim().toUpperCase().startsWith("F") || v.trim().equals("0")) {
            return false;
        }
        else {
            log.severe(String.format("Conversion error of value: %s", v));
            return null;
        }
    }

    /**
     * Return as a Boolean the value associated with a specified key if it's not null, otherwise
     * return the default value.
     *
     * @param key the key for the desired value.
     * @param defaultValue the value to return if the key does not exist.
     * @return the value associated with the key, or defaultValue if the key does not exist.
     */
    public static synchronized Boolean getBooleanValue (String key, Boolean defaultValue)
    {
        Boolean value = getBooleanValue (key);
        return value != null ? value : defaultValue;
    }

    /**
     * Return as an Integer the value associated with a specified key.
     *
     * @param key the key for the desired value.
     * @return the value associated with the key, or null if the key does not exist or is not an Integer or string
     *         representation of an Integer.
     */
    public static synchronized Integer getIntegerValue (String key)
    {
        String v = getStringValue(key);
        if (v == null)
            return null;

        try {
            return Integer.parseInt(v);
        }
        catch (NumberFormatException e) {
            log.severe(String.format("Conversion error of value: %s", v));
            return null;
        }
    }

    /**
     * Return as a Integer the value associated with a specified key if it's not null, otherwise
     * return the default value.
     *
     * @param key the key for the desired value.
     * @param defaultValue the value to return if the key does not exist.
     * @return the value associated with the key, or defaultValue if the key does not exist.
     */
    public static synchronized Integer getIntegerValue (String key, Integer defaultValue)
    {
        Integer value = getIntegerValue (key);
        return value != null ? value : defaultValue;
    }

    /**
     * Return as an Long the value associated with a specified key.
     *
     * @param key the key for the desired value.
     * @return the value associated with the key, or null if the key does not exist or is not a Long or string
     *         representation of a Long.
     */
    public static synchronized Long getLongValue (String key)
    {
        String v = getStringValue(key);
        if (v == null)
            return null;

        try {
            return Long.parseLong(v);
        }
        catch (NumberFormatException e) {
            log.severe(String.format("Conversion error of value: %s", v));
            return null;
        }
    }

    /**
     * Return as a Long the value associated with a specified key if it's not null, otherwise
     * return the default value.
     *
     * @param key the key for the desired value.
     * @param defaultValue the value to return if the key does not exist.
     * @return the value associated with the key, or defaultValue if the key does not exist.
     */
    public static synchronized Long getLongValue (String key, Long defaultValue)
    {
        Long value = getLongValue (key);
        return value != null ? value : defaultValue;
    }

    /**
     * Return as an Double the value associated with a specified key.
     *
     * @param key the key for the desired value.
     * @return the value associated with the key, or null if the key does not exist or is not an Double or string
     *         representation of an Double.
     */
    public static synchronized Double getDoubleValue (String key)
    {
        String v = getStringValue(key);
        if (v == null)
            return null;

        try {
            return Double.parseDouble(v);
        }
        catch (NumberFormatException e) {
            log.severe(String.format("Conversion error of value: %s", v));
            return null;
        }
    }

    /**
     * Return as a Double the value associated with a specified key if it's not null, otherwise
     * return the default value.
     *
     * @param key the key for the desired value.
     * @param defaultValue the value to return if the key does not exist.
     * @return the value associated with the key, or defaultValue if the key does not exist.
     */
    public static synchronized Double getDoubleValue (String key, Double defaultValue)
    {
        Double value = getDoubleValue (key);
        return value != null ? value : defaultValue;
    }

    /**
     * Determines whether a key exists in the configuration.
     *
     * @param key the key of interest.
     * @return true if the key exists, otherwise false.
     */
    public static synchronized boolean hasKey (String key)
    {
        return getInstance()._properties.contains(key);
    }

    /**
     * Removes a key and its value from the configuration if the configuration contains the key.
     *
     * @param key the key of interest.
     */
    public static synchronized void removeKey (String key)
    {
        getInstance()._properties.remove(key);
    }

    /**
     * Adds a key and value to the configuration, or changes the value associated with the key if the key is already in
     * the configuration.
     *
     * @param key   the key to set.
     * @param value the value to associate with the key.
     */
    public static synchronized void setValue (String key, Object value)
    {
        getInstance()._properties.put(key, value.toString());
    }

    private Properties _properties;
    private static final Logger log = Logger.getLogger(Config.class.toString());

    private static Config INSTANCE = null;
}
