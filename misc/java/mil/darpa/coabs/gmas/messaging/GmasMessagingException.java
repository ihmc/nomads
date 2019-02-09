package mil.darpa.coabs.gmas.messaging;

/**
 * Exception generated in the course of GMAS Messaging.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version
 */
public class GmasMessagingException extends Exception {

    /**
     * 
     */
    public GmasMessagingException()
    {
        super();
    }

    /**
     * @param message
     */
    public GmasMessagingException (String message)
    {
        super (message);
    }

    /**
     * @param cause
     */
    public GmasMessagingException (Throwable cause)
    {
        super (cause);
    }

    /**
     * @param message
     * @param cause
     */
    public GmasMessagingException (String message, Throwable cause)
    {
        super (message, cause);
    }

    private static final long serialVersionUID = 1L;

}
