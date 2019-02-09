package us.ihmc.aci.grpMgrOld;

/**
 *
 *
 * @author Niranjan Suri
 * @version $Revision$
 * $Date$
 */
public class GroupManagerException extends Exception
{
    public GroupManagerException()
    {
    }

    /**
     * Constructor use when a group manager exception displays a message.
     *
     * @param msg    the error message
     */
    public GroupManagerException (String msg)
    {
        super (msg);
    }

    private static final long serialVersionUID = -1456693588481751827L;
}
