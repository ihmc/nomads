package us.ihmc.aci.nodemon;

/**
 * NodeMonException.java
 * <p/>
 * Class <code>MalformedNodeException</code> identifies errors within the read, write and structure of <code>NODE</code>
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class NodeMonException extends RuntimeException
{
    /**
     * Constructs a <code>NodeMonException</code> with no message.
     */
    public NodeMonException ()
    {
    }

    /**
     * Constructs a <code>NodeMonException</code> with the given message.
     *
     * @param msg description of exception
     */
    public NodeMonException (String msg)
    {
        super(msg);
    }

    /**
     * Constructs a <code>NodeMonException</code> with the given cause.
     *
     * @param e the reason of the exception.
     */
    public NodeMonException (Throwable e)
    {
        super(e);
    }

}