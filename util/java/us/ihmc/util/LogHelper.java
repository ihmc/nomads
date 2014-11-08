/*
 * LogHelper.java
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

/**
 * LogHelper.java
 *
 * Wrapper class for Java implementation of logging facility. This uses the
 * java.util.logging facility that is built into the API.
 *
 * @author      Matteo Rebeschini   <mrebeschini@ihmc.us>
 *              Marco Arguedas      <marguedas@ihmc.us>
 *
 *
 * @version     $Revision: 1.4 $
 *              $Date: 2014/11/07 17:58:06 $
 *
 */

package us.ihmc.util;

import java.io.IOException;
import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

/**
 *
 */
public class LogHelper
{
    private LogHelper()
    {
    } //intentionally declared private.

    /**
     *
     */
    public static synchronized Logger getLogger (String loggerName)
    {
        initLogger (loggerName);
        Logger logger = Logger.getLogger (loggerName);
        return logger;
    }

    /**
     *
     */
    public static synchronized Logger getLogger (String loggerName, ConfigLoader confLoader)
    {
        initLogger (loggerName, confLoader);
        return getLogger (loggerName);
    }

    /**
     *
     */
    public static synchronized void initLogger (String loggerName, ConfigLoader confLoader)
    {
        if (confLoader == null) {
            confLoader = ConfigLoader.getDefaultConfigLoader();
        }

        try {
            boolean enableLogger = confLoader.getPropertyAsBoolean (LOGGER_ENABLED_KEY,     true);
            String loggerDir     = confLoader.getProperty          (LOGGER_FILE_PATH_KEY,   "../logs");
            String logFilePrefix = confLoader.getProperty          (LOGGER_FILE_PREFIX_KEY, "log");
            int logFileSizeLimit = confLoader.getPropertyAsInt     (LOGGER_FILE_SIZE_KEY,   5242880);
            int numLogFiles      = confLoader.getPropertyAsInt     (LOGGER_FILE_NUMBER_KEY, 10);
            String logLevel      = confLoader.getProperty          (LOGGER_LEVEL_KEY,       "ALL");
            Level level = parseLevelString (logLevel);

            java.util.logging.Logger logger = java.util.logging.Logger.getLogger (loggerName);

            if (enableLogger) {
                Handler[] h = logger.getHandlers();
                for (int i = 0; i < h.length; i++) {
                    if (h[i] instanceof ConsoleHandler) {
                        logger.removeHandler (h[i]);
                    }
                }

                logger.setLevel (level);
                logger.setUseParentHandlers (false);

                ConsoleHandler ch = new ConsoleHandler();
                ch.setFormatter (new SimpleTextFormatter());
                ch.setLevel (level);
                logger.addHandler (ch);

                String pattern = loggerDir + "/" + logFilePrefix + "%g.log";
                FileHandler fh = new FileHandler (pattern, logFileSizeLimit, numLogFiles);
                fh.setLevel (level);
                fh.setFormatter (new SimpleTextFormatter());
                logger.addHandler (fh);
            }
        }
        catch (IOException ex) {
            ex.printStackTrace();
        }
        catch (SecurityException e) {
            e.printStackTrace();
        }
    }

    /**
     *
     */
    public static synchronized void initLogger (String loggerName, Level level, String logFileName)
    {
        java.util.logging.Logger logger = java.util.logging.Logger.getLogger (loggerName);

        Handler[] h = logger.getHandlers();
        for (int i = 0; i < h.length; i++) {
            if (h[i] instanceof ConsoleHandler) {
                logger.removeHandler (h[i]);
            }
        }

        logger.setLevel (level);
        logger.setUseParentHandlers (false);

        ConsoleHandler ch = new ConsoleHandler();
        ch.setFormatter (new SimpleTextFormatter());
        ch.setLevel (level);
        logger.addHandler (ch);

        try {
            String pattern = logFileName;
            FileHandler fh = new FileHandler (pattern);
            fh.setLevel (level);
            fh.setFormatter (new SimpleTextFormatter());
            logger.addHandler (fh);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    /**
     *
     */
    public static synchronized void initLogger (String loggerName)
    {
        java.util.logging.Logger logger = java.util.logging.Logger.getLogger (loggerName);

        Handler[] h = logger.getHandlers();
        for (int i = 0; i < h.length; i++) {
            if (h[i] instanceof ConsoleHandler) {
                logger.removeHandler (h[i]);
            }
        }

        logger.setLevel (Level.ALL);
        logger.setUseParentHandlers (false);

        ConsoleHandler ch = new ConsoleHandler();
        ch.setFormatter (new SimpleTextFormatter());
        ch.setLevel (Level.ALL);
        logger.addHandler (ch);
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     *
     */
    private static Level parseLevelString (String strLevel)
    {
        try {
            Level level = Level.parse (strLevel);
            return level;
        }
        catch (Exception ex) {
            return Level.ALL;
        }
    }


    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     *
     */
    private static class SimpleTextFormatter extends Formatter
    {
        public String format (LogRecord logRec)
        {
            StringBuffer sb = new StringBuffer(100);

            sb.append (logRec.getSourceClassName());
            sb.append (".");
            sb.append (logRec.getSourceMethodName());
            sb.append (" ");
            sb.append (logRec.getMessage());
            sb.append (EOL);

            return sb.toString();
        }

        public static final String EOL = System.getProperty ("line.separator");
    } //class SimpleTextFormatter

    // /////////////////////////////////////////////////////////////////////////
    private static final String LOGGER_ENABLED_KEY     = "logger.enabled";
    private static final String LOGGER_FILE_PATH_KEY   = "logger.file.path";
    private static final String LOGGER_FILE_PREFIX_KEY = "logger.file.prefix";
    private static final String LOGGER_FILE_SIZE_KEY   = "logger.file.size";
    private static final String LOGGER_FILE_NUMBER_KEY = "logger.file.number";
    private static final String LOGGER_LEVEL_KEY       = "logger.level.key";
} //class Logger
