package us.ihmc.aci.grpMgrOld;

/**
 * JoinInfo is the base class used to represent joining a group
 *
 * @author Niranjan Suri
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class JoinInfo
{
    // Join modes
    public static final byte JOIN_ONE_TIME_ONLY = 0x00000001;
    public static final byte JOIN_AFTER_CONFIRMATION = 0x00000002;
    public static final byte JOIN_ALWAYS = 0x00000003;

    public String groupName;
    public String creatorUUID;
    public byte[] joinData;
    public int joinDataLen;
    public int joinMode;
}

