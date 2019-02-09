package us.ihmc.aci.kernel;

public class ServiceEvent
{
    public static final int UNDEFINED = 0x00;
    public static final int NEW_PEER = 0x01;                          // uses nodeUUID
    public static final int DEAD_PEER = 0x02;                         // uses nodeUUID
    public static final int GROUP_LIST_CHANGE = 0x03;                 // uses nodeUUID
    public static final int NEW_GROUP_MEMBER = 0x04;                  // uses groupName, memberUUID, data
    public static final int GROUP_MEMBER_LEFT = 0x05;                 // uses groupName, memberUUID
    public static final int CONFLICS_WITH_PRIVATE_PEER_GROUP = 0x06;  // uses groupName, nodeUUID
    public static final int PEER_GROUP_DATA_CHANGED = 0x07;           // uses groupName, nodeUUID, data
    public static final int PEER_SEARCH_REQUEST_RECEIVED = 0x08;      // uses groupName, nodeUUID, searchUUID, param
    public static final int PEER_SEARCH_RESULT_RECEIVED = 0x09;       // uses groupName, nodeUUID, searchUUID, param
    public static final int PEER_MESSAGE_RECEIVED = 0x10;             // uses groupName, nodeUUID, data
    public static final int PERSISTENT_PEER_SEARCH_TERMINATED = 0x11; // uses groupName, nodeUUID, searchUUID

    public int eventType;
    public String nodeUUID;
    public String groupName;
    public String memberUUID;
    public String searchUUID;
    byte[] data;
    byte[] param;
}
