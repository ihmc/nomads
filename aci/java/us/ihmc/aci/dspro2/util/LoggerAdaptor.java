package us.ihmc.aci.dspro2.util;

import java.util.logging.Level;

/**
 *
 * @author gbenincasa
 */
public class LoggerAdaptor implements LoggerInterface
{
    public LoggerAdaptor (Class clazz)
    {
        _logger = java.util.logging.Logger.getLogger(clazz.getSimpleName());
    }

    public void configure(String loggerConfig)
    {
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
        _logger.warning(message);
    }

    public void error (String message)
    {
        _logger.log(Level.SEVERE, message);
    }

    public void error (Throwable t)
    {
        _logger.log(Level.SEVERE, "ERROR", t);
    }

    public void error (String message, Throwable t)
    {
        _logger.log(Level.SEVERE, message, t);
    }

    private final java.util.logging.Logger _logger;
}
