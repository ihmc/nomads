package us.ihmc.aci.dspro2.util;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;

/**
 *
 * @author gbenincasa
 */
public class Log4jAdaptor implements LoggerInterface
{
    public Log4jAdaptor (Class clazz)
    {
        _logger = org.apache.log4j.Logger.getLogger(clazz);
    }

    public void configure(String loggerConfig)
    {
        Properties log4jProperties = getLog4jProperties (loggerConfig);
        org.apache.log4j.PropertyConfigurator.configure (log4jProperties);
    }

    public void info (String message)
    {
        _logger.info(message);
    }

    public void debug (String message)
    {
        _logger.info(message);
    }

    public void warn (String message)
    {
        _logger.warn(message);
    }

    public void error (String message)
    {
        _logger.error(message);
    }

    public void error (Throwable t)
    {
        _logger.error("ERROR", t);
    }

    public void error (String message, Throwable t)
    {
        _logger.error(message, t);
    }

    private Properties getLog4jProperties (String configFilePath)
    {
        Properties log4jProperties = new Properties();
        try {
            log4jProperties.load (new FileInputStream (configFilePath));
            String logFileName = log4jProperties.getProperty ("log4j.appender.rollingFile.File");
            Date day = new Date();
            String formattedDate = new SimpleDateFormat ("yyyyMMddhhmm").format (day);
            log4jProperties.setProperty ("log4j.appender.rollingFile.File", String.format ("../logs/%s-%s",
                    formattedDate, logFileName));
        }
        catch (FileNotFoundException e) {
            error ("Unable to load log4j configuration, file not found", e);
        }
        catch (IOException e) {
            error ("Unable to load log4j configuration, error while I/O on disk", e);
        }
        return log4jProperties;
    }

    private final org.apache.log4j.Logger _logger;
}
