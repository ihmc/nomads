package us.ihmc.aci.netSupervisor.inferenceModule;

public class ClaspGroup
{

    private int _groupId;
    private String _groupName;
    private String _subGroup;
    private String _mainGroup;

    public ClaspGroup()
    {
        _groupId = 0;
        _groupName = "";
        _subGroup = "";
        _mainGroup = "";
    }

    public int getGroupId() {
        return _groupId;
    }

    public void setGroupId(int groupId) {
        _groupId = groupId;
    }

    public String getGroupName() {
        return _groupName;
    }

    public void setGroupName(String groupName)
    {
        _groupName = groupName;
    }

    public String getSubGroup()
    {
        return _subGroup;
    }

    public String getMainGroup()
    {
        return _mainGroup;
    }

    public String print ()
    {
        return "(" + _groupId + "," + _mainGroup + "," + _subGroup + "," + _groupName + ")";
    }

    public void setSubGroup(String subGroup)
    {
        _subGroup = subGroup;
    }

    public void setMainGroup(String mainGroup)
    {
        _mainGroup = mainGroup;
    }

    public void setLinkGroup ()
    {
        _mainGroup = "link";
        _subGroup = "linkIps";
    }
}
