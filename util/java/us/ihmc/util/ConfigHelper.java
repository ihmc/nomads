/*
 * ConfigHelper.java
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
import java.io.FileOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.Properties;

/**
 * @deprecated
 * @author Tom Cowin <tcowin@ihmc.us>
 *  Simple class to get properties for agent system operations from cfg file located on disk.
 * $Date: 2014/11/07 17:58:06 $
 * $Release: $
 */
public class ConfigHelper
{
    private static String getConfigFileSpec() throws IOException
    {
        String cfgFile = "nomads.cfg";

        if (null != (_homeDir = System.getProperty ("spring.home"))) {
            cfgFile = "spring.cfg";
        }
        else if (null != (_homeDir = System.getProperty ("mast.home"))) {
            cfgFile = "mastk.cfg";
        }
        else if (null == (_homeDir = System.getProperty ("nomads.home"))) {
            throw new IOException ("Home dir not found; set Java System Property mast.home or nomads.home on command line like eg.: -Dmast.home=c:\\Program Files\\IHMC MAST");
        }
        if (!(new File (_homeDir)).isDirectory())
            throw new IOException ("HomeDir as specified: <" + _homeDir + "> doesn't exist");

        // Determine path to mastk config file
        return _homeDir + File.separator + "conf" + File.separator + cfgFile;
    }

    public static Properties getSystemProperties() throws IOException
    {
        // Try to read the cfg file if present
        Properties systemProperties = new Properties();
        try {
            systemProperties.load (new FileInputStream (getConfigFileSpec()));
        }
        catch (Exception e) {
            System.out.println ("Warning: problem reading cfg file: " + getConfigFileSpec());
            e.printStackTrace();
        }
        systemProperties.put ("HomeDir", _homeDir);

        return systemProperties;
    }

    public static void putSystemProperties (Properties properties) throws IOException
    {

        try {
            properties.store (new FileOutputStream (getConfigFileSpec()), null);
        }
        catch (Exception e) {
            System.out.println ("Warning: problem writing cfg file: " + getConfigFileSpec());
            e.printStackTrace();
        }
    }

    private static String _homeDir = null;
}
