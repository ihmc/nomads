/**
 * GroupManagerProxyJSMImpl.java
 *
 * @author      Matteo Rebeschini   <mrebeschini@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.kernel;

import us.ihmc.aci.grpMgrOld.GroupManagerProxy;

/**
 *
 */
public class GroupManagerProxyJSMImpl extends GroupManagerProxy
{
    public native String getNodeUUID();
    public native String getNodeName (String nodeUUID);
    public native void createPublicPeerGroup (String groupName, byte[] data);
    public native void createPrivatePeerGroup (String groupName, String password, byte[] data);
    public native String startPersistentPeerSearch (String groupName, byte[] param);
    public native void stopPersistentPeerSearch (String searchUUID);
    public native void respondToPeerSearch (String searchUUID, byte[] param);
    public native void sendPeerMessage (String nodeUUID, byte[] data, boolean reliable, int timeout);
    public native void broadcastPeerMessageToGroup (String groupName, byte[] data);
    public native void updatePeerGroupData (String groupName, byte[] data);
    public native void statusChanged();
    public native void removeGroup (String groupName);
    public native void createPublicManagedGroup (String groupName);
    public native void createPrivateManagedGroup (String groupName, String password);
    public native void joinPublicManagedGroup (String groupName, String creatorUUID);
    public native void joinPrivateManagedGroup (String groupName, String creatorUUID, String password);
    public native void leaveGroup (String groupName);

    /**
     * @param msg String
     */
    private static void log (String msg)
    {
        if (DEBUG) {
            System.out.println ("[GrpMgrProxyJSMImpl] " + msg);
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    private static final boolean DEBUG = false;

    // /////////////////////////////////////////////////////////////////////////
    // STATIC INITIALIZER //////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary ("acinative");
    }
}

