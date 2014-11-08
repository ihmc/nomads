package TestPushPro;


import us.ihmc.aci.disServiceProProxy.DisServiceProProxy;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyListener;
import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.disServiceProProxy.NodePath;
import us.ihmc.aci.disServiceProxy.PeerStatusListener;

/**
 *
 * @author mmarchini
 */
public class ClientB implements DisServiceProProxyListener, MatchmakingLogListener, PeerStatusListener, Runnable {

    static private DisServiceProProxy disPro;
    
    public ClientB() {}
    
    public void run() {
        
        
        disPro.setActualPosition((float) 11.5, (float) 11.5, 0, null, null);
        System.out.println("Position set to 11.5 , 11.5");
        
        System.out.println("Sleep for 3 seconds...");
        try { Thread.sleep(3000); }
            catch (Exception e) { }
        
        disPro.setActualPosition((float) 10.5, (float) 10.5, 0, null, null);
        System.out.println("Position set to 10.5 , 10.5");
    }
    
    public static void main(String[] args) {
        
        ClientB clientB = new ClientB();
        disPro = new DisServiceProProxy((short)1);
        disPro.init();
        disPro.registerDisServiceProProxyListener(clientB);
        disPro.registerMatchmakingLogListener(clientB);
        disPro.registerPeerStatusListener(clientB);
        
        (new Thread(clientB)).start();
    }

    public boolean pathRegistered(NodePath path, String nodeId, String teamId, String mission) {
        System.out.println("Path registered");
        return true;
    }

    public boolean positionUpdated(float latitude, float longitude, float altitude, String nodeId) {
        System.out.println("Position updated: latitude = "+ latitude + "; longitude = " + longitude + "; altitude = " + altitude);
        return true;
    }

    public boolean dataArrived(String sender, String groupName, int seqNum, byte[] data, int metadataLength, short historyWindow, short tag, byte priority) {
        System.out.println("Data arrived");
        return true;
    }

    public boolean chunkArrived(String sender, String groupName, int seqNum, byte[] data, short historyWindow, short tag, byte priority) {
        System.out.println("Chunk arrived");
        return true;
    }

    public boolean metadataArrived(String sender, String groupName, int seqNum, byte[] metadata, short historyWindow, short tag, byte priority) {
        System.out.println("Metadata arrived");
        return true;
    }

    public boolean dataAvailable(String sender, String groupName, int seqNum, String id, byte[] metadata, short historyWindow, short tag, byte priority) {
        System.out.println("Data available");
        return true;
    }

    public void informationMatched(String localNodeID, String peerNodeID, String matchedObjectID, String matchedObjectName, String[] rankDescriptors, float[] partialRanks, float[] weights, String comment, String operation) {
        System.out.println("Information matched");
    }

    public void informationSkipped(String localNodeID, String peerNodeID, String skippedObjectID, String skippedObjectName, String[] rankDescriptors, float[] partialRanks, float[] weights, String comment, String operation) {
        System.out.println("Information skipped");
    }

    public boolean newPeer(String peerID) {
        System.out.println("New peer");
        return true;
    }

    public boolean deadPeer(String peerID) {
        System.out.println("Dead peer");
        return true;
    }
}
