package us.ihmc.aci.util.dspro.test;

import java.io.Serializable;

/**
 *
 * @author gbenincasa
 */
public class Message implements Serializable
{
    public final String _id;
    public final String _groupName;
    public final String _objectId;
    public final String _instanceId;
    public final String _xmlMedatada;
    public final byte[] _data;
    public final long _expirationTime;

    public Message (String id, String groupName, String objectId, String instanceId,
                    String xmlMedatada, byte[] data, long expirationTime)
    {
        _id = id;
        _groupName = groupName;
        _objectId = objectId;
        _instanceId = instanceId;
        _xmlMedatada = xmlMedatada;
        _data = data;
        _expirationTime = expirationTime;
    }
}
