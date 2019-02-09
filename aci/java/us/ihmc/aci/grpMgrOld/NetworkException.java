package us.ihmc.aci.grpMgrOld;

/**
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class NetworkException extends Exception
{
    public NetworkException()
    {
    }

    /**
     * @param msg    the error message
     */
    public NetworkException (String msg)
    {
        super (msg);
    }

    private static final long serialVersionUID = -8340314998872808460L;
}
