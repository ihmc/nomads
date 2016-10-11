package us.ihmc.aci.dspro2.legacy;

import java.util.Collection;

/**
 *
 * @author Giacomo
 */
public class LegacySearchListener implements us.ihmc.aci.dspro2.SearchListener
{
    private final us.ihmc.aci.disServiceProxy.SearchListener _listener;

    LegacySearchListener (us.ihmc.aci.disServiceProxy.SearchListener listener)
    {
        _listener = listener;
    }
    
    public void searchArrived(String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query)
    {
        _listener.searchArrived(queryId, groupName, querier, queryType, queryQualifiers, query);
    }

    public void searchReplyArrived(String queryId, Collection<String> matchingMessageIds, String responderNodeId)
    {
        _listener.searchReplyArrived(queryId, matchingMessageIds, responderNodeId);
    }

    public void searchReplyArrived(String queryId, byte[] reply, String responderNodeId)
    {
        _listener.searchReplyArrived(queryId, reply, responderNodeId);
    }
    
}
