package us.ihmc.aci.kernel;

import java.io.IOException;

import us.ihmc.aci.grpMgrOld.GroupManagerListener;

class RVMServiceEventHandler extends Thread
{

    private static final boolean DEBUG = true;
    private RVMServiceEventReader _reader;
    private  GroupManagerProxyRVMImpl _gmp;
    private boolean _stop;

    RVMServiceEventHandler (RVMServiceEventReader r, GroupManagerProxyRVMImpl gmp)
    {
        _reader = r;
        _stop = false;
        _gmp = gmp;
    }

    void stopHandler()
    {
        _stop = true;
    }

    /**
     *
     */
    public void run()
    {
        ServiceEvent se;
        while(!_stop)
        {
            try {
                se = (ServiceEvent) _reader.getNextEvent();
                if (se != null) {
                    processEvent(se);
                }
            }
            catch (IOException e) {
                // now, just do nothing
            }
        }
    }

    /**
    *
    */
   public void processEvent (ServiceEvent se)
   {
       GroupManagerListener gml = _gmp.getListener();

       if (gml == null) {
           return;
       }

       log("Received event of type "+se.eventType);

       switch (se.eventType) {
           case ServiceEvent.NEW_PEER :
               gml.newPeer (se.nodeUUID);
               break;

           case ServiceEvent.DEAD_PEER :
               gml.deadPeer (se.nodeUUID);
               break;

           case ServiceEvent.GROUP_LIST_CHANGE :
               gml.groupListChange (se.nodeUUID);
               break;

           case ServiceEvent.NEW_GROUP_MEMBER :
               gml.newGroupMember (se.groupName, se.memberUUID, se.data);
               break;

           case ServiceEvent.GROUP_MEMBER_LEFT :
               gml.groupMemberLeft (se.groupName, se.memberUUID);
               break;

           case ServiceEvent.CONFLICS_WITH_PRIVATE_PEER_GROUP :
               gml.conflictWithPrivatePeerGroup(se.groupName, se.nodeUUID);
               break;

           case ServiceEvent.PEER_GROUP_DATA_CHANGED :
               gml.peerGroupDataChanged (se.groupName, se.nodeUUID, se.data);
               break;

           case ServiceEvent.PEER_SEARCH_REQUEST_RECEIVED :
               gml.peerSearchRequestReceived (se.groupName, se.nodeUUID, se.searchUUID, se.param);
               break;

           case ServiceEvent.PEER_SEARCH_RESULT_RECEIVED :
               gml.peerSearchResultReceived (se.groupName, se.nodeUUID, se.searchUUID, se.param);
               break;

           case ServiceEvent.PERSISTENT_PEER_SEARCH_TERMINATED :
               gml.persistentPeerSearchTerminated (se.groupName, se.nodeUUID, se.searchUUID);
               break;

           default:
               log ("processEvent:: WARNING: Unknown type for event.");
       }
   }

   private static void log (String msg)
   {
       if (DEBUG) {
           System.out.println("[RVMServiceEventHandler] " + msg);
       }
   }

}
