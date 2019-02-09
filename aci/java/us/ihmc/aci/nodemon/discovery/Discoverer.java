package us.ihmc.aci.nodemon.discovery;

/**
 * Discoverer.java
 *
 * Interface <code>Discoverer</code> contains common methods to implement a discovery service for <code>NodeMon</code>.
 */
public interface Discoverer {

    void newNode (String nodeId);

    void deadNode (String nodeId);

    void groupListChange (String nodeUUID);

    void newGroupMember (String groupName, String memberUUID);
}
