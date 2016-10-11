package us.ihmc.aci.dspro2.util;

/**
 * A simple wrapper to make java.util.logging API compatible with log4j for easier switching.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class LoggerWrapper
{
    public enum Type {
        STANDARD,
        LOG4J
    }
            
    private LoggerWrapper (LoggerInterface logger)
    {
        _logger = logger;
    }

    public static LoggerInterface getLogger (Class clazz)
    {
        return getLogger (Type.STANDARD, clazz);
    }

    public static LoggerInterface getLogger (Type type, Class clazz)
    {
        switch (type) {
            case LOG4J:
                return new Log4jAdaptor(clazz);

            default:
                return new LoggerAdaptor(clazz);
        }
    }

    private static LoggerInterface _logger;
}
