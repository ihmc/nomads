package us.ihmc.aci.grpMgrOld.gui;

/**
 * An object that contains the groupName, status and membership.
 * 
 * @author Maggie Breedy <NOMADS team>
 * 
 * @version $Revision$
 */

public class GroupObject
{
    public GroupObject (String groupName, String status, String member, String parsedName)
    {
        _groupName = groupName;
        _status = status;
        _member = member;
        _parsedName = parsedName;
    }
    
    public String toString()
    {
        return _parsedName;
    }
    
    public String _groupName;
    public String _status;
    public String _member;
    public String _parsedName;
}
