package mil.darpa.coabs.gmas;

/**
 * Exception generated in the course of Basic GMAS Service execution.
 * <p>
 * 
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version
 */
public class GmasServiceException extends Exception {

    /**
     * 
     */
    public GmasServiceException()
    {
        super();
    }

    /**
     * @param message
     */
    public GmasServiceException (String message)
    {
        super (message);
    }

    /**
     * @param cause
     */
    public GmasServiceException (Throwable cause)
    {
        super (cause);
    }

    /**
     * @param message
     * @param cause
     */
    public GmasServiceException (String message, Throwable cause)
    {
        super (message, cause);
    }

    private static final long serialVersionUID = 1L;

}
