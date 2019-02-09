package us.ihmc.aci.netviewer.proxy;

import us.ihmc.aci.nodemon.data.traffic.TrafficParticle;

/**
 * Container for neighbor link info
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class NeighborLinkInfo
{
    /**
     * Constructor
     * @param linkInfo link info
     * @param incoming the incoming traffic value
     * @param outgoing the outgoing traffic value
     */
    private NeighborLinkInfo (String linkInfo, TrafficParticle incoming, TrafficParticle outgoing)
    {
        _linkInfo = linkInfo;
        _incoming = incoming;
        _outgoing = outgoing;
    }

    public static NeighborLinkInfo create (String linkInfo, TrafficParticle incoming, TrafficParticle outgoing)
    {
        return new NeighborLinkInfo (linkInfo, incoming, outgoing);
    }

    public static NeighborLinkInfo create (String linkInfo)
    {
        return new NeighborLinkInfo (linkInfo, null, null);
    }

    /**
     * Gets the link info
     * @return the link info
     */
    public String getLinkInfo()
    {
        return _linkInfo;
    }

    /**
     * Gets incoming traffic info
     * @return the incoming traffic value
     */
    public TrafficParticle getIncomingTraffic()
    {
        return _incoming;
    }

    /**
     * Gets outgoing traffic info
     * @return the outgoing traffic value
     */
    public TrafficParticle getOutgoingTraffic()
    {
        return _outgoing;
    }


    private final String _linkInfo;
    private final TrafficParticle _incoming;
    private final TrafficParticle _outgoing;
}
