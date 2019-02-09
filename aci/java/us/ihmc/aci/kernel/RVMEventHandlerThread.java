package us.ihmc.aci.kernel;

import us.ihmc.aci.grpMgrOld.GroupManagerProxy;

public class RVMEventHandlerThread extends Thread {

    private GroupManagerProxy _gmp;

    public RVMEventHandlerThread(GroupManagerProxy gmp) {
        setName("JavaEventHandlerThread-[" + getName() + "]");
        _gmp = gmp;
    }

    public void run() {
        ServiceEvent se = new ServiceEvent();

        while (true) {
            try {

                while(EmbeddedRVMServiceManager.getNextEvent(se) == -1)
                {
                    try {
                        Thread.sleep(200);
                    } catch (InterruptedException e) {

                    }
                }

                processEvent(se);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void processEvent (ServiceEvent se)
    {
        switch (se.eventType) {
            case ServiceEvent.NEW_PEER :
                _gmp.newPeer (se.nodeUUID);
                break;

            case ServiceEvent.DEAD_PEER :
                _gmp.deadPeer (se.nodeUUID);
                break;

            case ServiceEvent.GROUP_LIST_CHANGE :
                _gmp.groupListChange (se.nodeUUID);
                break;

            case ServiceEvent.NEW_GROUP_MEMBER :
                _gmp.newGroupMember (se.groupName, se.memberUUID, se.data);
                break;

            case ServiceEvent.GROUP_MEMBER_LEFT :
                _gmp.groupMemberLeft (se.groupName, se.memberUUID);
                break;

            case ServiceEvent.CONFLICS_WITH_PRIVATE_PEER_GROUP :
                _gmp.conflictWithPrivatePeerGroup (se.groupName, se.nodeUUID);
                break;

            case ServiceEvent.PEER_GROUP_DATA_CHANGED :
                _gmp.peerGroupDataChanged (se.groupName, se.nodeUUID, se.data);
                break;

            case ServiceEvent.PEER_SEARCH_REQUEST_RECEIVED :
                _gmp.peerSearchRequestReceived (se.groupName, se.nodeUUID, se.searchUUID, se.param);
                break;

            case ServiceEvent.PEER_SEARCH_RESULT_RECEIVED :
                _gmp.peerSearchResultReceived (se.groupName, se.nodeUUID, se.searchUUID, se.param);
                break;

            case ServiceEvent.PEER_MESSAGE_RECEIVED :
                _gmp.peerMessageReceived (se.groupName, se.nodeUUID, se.data);

            case ServiceEvent.PERSISTENT_PEER_SEARCH_TERMINATED :
                _gmp.persistentPeerSearchTerminated (se.groupName, se.nodeUUID, se.searchUUID);
                break;

            default:
                log ("processEvent:: WARNING: Unknown type for event.");
        }
    } //processEvent

    private static final boolean DEBUG = true;

    private static void log(String msg) {
        if (DEBUG) {
            System.out.println("[RVMEventHandlerThread] " + msg);
        }
    }

}
