package us.ihmc.aci.dspro2.util;

/**
 *
 * @author gbenincasa
 */
public interface LoggerInterface
{
    public void configure(String loggerConfig);
    public void info (String message);
    public void debug (String message);
    public void warn (String message);
    public void error (String message);
    public void error (Throwable t);
    public void error (String message, Throwable t);
}
