/*
 * DSProLogger.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import java.io.IOException;
import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import us.ihmc.util.StringUtil;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class DSProLogger
{
    public final static Logger LOGGER = Logger.getLogger(DSProLogger.class.getName());
    static {
        try {
            DSProLogger.setup (LOGGER, "dsproproxy.log", false);
        }
        catch (IOException ex) {
            System.err.println (DSProLogger.class.getName() +
                                " could not initialize logger:\n" +
                                StringUtil.getStackTraceAsString(ex));
            System.exit(1);
        }
    }

    static public void setup (Logger logger, String logFile, boolean debug)
        throws IOException
    {
        if (debug) {
            logger.setLevel (Level.FINEST);
            FileHandler fileTxt = new FileHandler (logFile);
            logger.addHandler (fileTxt);
        }
        else {
            logger.setLevel (Level.INFO);
        }

        logger.setUseParentHandlers (false);
        logger.addHandler (new ConsoleHandler());
        for (Handler handler : logger.getHandlers()) {
            handler.setFormatter (new Formatter() {
                private final String LINE_SEPARATOR = System.getProperty ("line.separator");

                @Override
                public String format(LogRecord record) {
                    StringBuilder msg;
                    if (record.getLevel() == Level.INFO) {
                        msg = new StringBuilder (record.getMessage());
                        msg.append (LINE_SEPARATOR);
                    }
                    else {
                        msg = new StringBuilder ("[");
                        msg.append (record.getLevel())
                           .append ("] ")
                           .append (record.getSourceClassName())
                           .append (".")
                           .append (record.getSourceMethodName())
                           .append (": ")
                           .append (record.getMessage())
                           .append (LINE_SEPARATOR);
                    }

                    return msg.toString();
                }
            });
        }
    }
}
