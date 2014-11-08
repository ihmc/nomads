/*
 * LoggerWrapper.java
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

import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.LogRecord;

import java.io.*;

/**
 * LoggerWrapper allows to read debug information from a file.
 * It reads properties from an specified logger config file to
 * initilize the Logger.
 *
 * @author  Maggie Breedy <Nomads team>
 * @version $Revision: 1.5 $
 *
 **/

public class LoggerWrapper
{
    public LoggerWrapper()
    {
    }
    
    public void initialize (ConfigLoader loader)
    {
        try {            
            System.out.println ("->LoggerWrapper:loader: " + loader.toString());
            if ((loader == null) || (loader.getProperty ("LoggerTopLevelPackage") == null)) {
                String loggerDir = System.getProperty ("user.dir") + System.getProperty ("file.separator") + "logger.cfg";
                System.out.println ("->LoggerWrapper:loggerDir: " + loggerDir);
                loader = ConfigLoader.getDefaultConfigLoader();
            }
            
            String levelStr = (String) loader.getProperty ("LoggerLevel");
            //System.out.println ("->LoggerWrapper:levelStr: " + levelStr);
            String logDir = (String) loader.getProperty ("LoggerTopLevelPackage");
            //System.out.println ("->LoggerWrapper:logDir: " + logDir);
            String logFileName = (String) loader.getProperty ("LoggerFileName");
            
            Logger logger = java.util.logging.Logger.getLogger (logDir);
            
            Level level = getLevelFromString (levelStr);
            logger.setLevel (level);
            
            FileHandler fh = new FileHandler (logFileName + "%g.log");
            fh.setLevel (level);
            fh.setFormatter (new LogFormatter());
            logger.addHandler (fh);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    private Level getLevelFromString (String level)
    {
        if (level.equalsIgnoreCase ("FINE")) {
            return Level.FINE;
        }
        else if (level.equalsIgnoreCase ("CONFIG")) {
            return Level.CONFIG;
        }
        else if (level.equalsIgnoreCase ("INFO")) {
            return Level.INFO;
        }
        else if (level.equalsIgnoreCase ("WARNING")) {
            return Level.WARNING;
        }
        else if (level.equalsIgnoreCase ("SEVERE")) {
            return Level.SEVERE;
        }
        else {
            return Level.WARNING;
        }
    }
    
    public static class LogFormatter extends Formatter
    {
        public String format (LogRecord record)
        {
            String s = "Logger: " + record.getMessage() + System.getProperty ("line.separator");
            Throwable t = record.getThrown();
            if (t != null) {
                try {
                    StringWriter sw = new StringWriter();
                    PrintWriter pw = new PrintWriter (sw);
                    t.printStackTrace (pw);
                    pw.close();
                    s += sw.toString();
                } 
                catch (Exception ex) {
                }
            }
            return s;
        }
    }
}
