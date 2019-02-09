package mil.darpa.coabs.gmas.mobility;

import java.lang.Exception;

/**
 * Exception generated in the course of GMAS Mobility.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version
 */
public class GmasMobilityException extends Exception

{

    public GmasMobilityException()
    {
    }

    public GmasMobilityException (String msg)
    {
        super (msg);
    }

    /**
     * @param cause
     */
    public GmasMobilityException (Throwable cause)
    {
        super (cause);
    }

    /**
     * @param message
     * @param cause
     */
    public GmasMobilityException (String message, Throwable cause)
    {
        super (message, cause);
    }    
    
    private static final long serialVersionUID = 1L;
}