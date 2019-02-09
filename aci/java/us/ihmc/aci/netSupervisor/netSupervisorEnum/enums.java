package us.ihmc.aci.netSupervisor.netSupervisorEnum;

/**
 * Created by Roberto on 4/27/2016.
 */
public class enums
{
    public enum trend
    {
        POSITIVE,
        NEGATIVE,
        STABLE,
        UNKNOWN,
    }

    public enum EventType
    {
        newNode,
        updatedNode,
        deadNode
    }

    public enum ConnectionType
    {
        GOOD,
        BAD,
        GOOD_CONGESTED,
        GOOD_PACKET_LOSS,
        BAD_CONGESTED,
        BAD_PACKET_LOSS,
        NOT_NEIGHBOURS,
        UNKNOWN;
    }

}
