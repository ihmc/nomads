package us.ihmc.aci.grpMgrOld;

import java.util.Hashtable;


/**
 * GroupInfo is the base class that represents a locally created group
 *
 * @author Niranjan Suri
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class GroupInfo implements Comparable
{
    public GroupInfo()
    {
        members = new Hashtable();
    }

    public int compareTo (Object o)
    {
        return groupName.compareToIgnoreCase (((GroupInfo)o).groupName);
    }

    public int floodProbability;
    public int hopCount;
    public String groupName;
    public Hashtable members;    // Maps UUIDs to GroupMemberInfo (or subclass) objects
}
