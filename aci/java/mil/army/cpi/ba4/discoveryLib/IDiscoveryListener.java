package mil.army.cpi.ba4.discoveryLib;

public interface IDiscoveryListener {

    void newGroupMember(String groupName, String memberUUID);

    void groupMemberLeft(String groupName, String memberUUID);

    void memberDataChanged(String memberUUID);

    void unbindFromDiscovery();

    void hopCountChanged(int hopCount);

    void peerSearchResponse(String nodeUUID, String searchUUID, int serviceType);

    void peerUnhealthy(String nodeUUID);
}
