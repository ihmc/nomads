package us.ihmc.aci.netSupervisor.inferenceModule;

public class ClaspHostDescription
{

    private String _hostIp;
    private String _groupId;
    private String _eventId;

    public ClaspHostDescription ()
    {

    }

    public ClaspHostDescription (String hostIp, String groupId, String eventId)
    {
        _hostIp = hostIp;
        _groupId = groupId;
        _eventId = eventId;
    }
    public String getEventId()
    {
        return _eventId;
    }

    public String getHostIp ()
    {
        return _hostIp;
    }

    public String getGroupId ()
    {
        return _groupId;
    }

    public void setEventId(String _eventId)
    {
        this._eventId = _eventId;
    }

    public void setHostIp (String hostIp)
    {
        _hostIp = hostIp;
    }

    public void setGroupId (String groupId)
    {
        _groupId = groupId;
    }
}
