package TestPushPro;


import java.util.ArrayList;
import java.util.List;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxy;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyListener;
import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.disServiceProProxy.NodePath;
import us.ihmc.aci.disServiceProxy.PeerStatusListener;

/**
 *
 * @author mmarchini
 */
public class ClientA implements DisServiceProProxyListener, MatchmakingLogListener, PeerStatusListener, Runnable {

    static private DisServiceProProxy disPro;
    
    public ClientA(){}
    
    public void run() {
        
        short idClient = 0;
        byte[] data = new byte[1024];
        String groupName = "ONR_Demo";
        long expirationTime = 0;
        short historyWindow = 0;
        short tag = 0;
        
        System.out.println("Setting metadata names..");
        List<String> metaDataAttributes = new ArrayList<String>(); 
        metaDataAttributes.add("LEFT_UPPER_LATITUDE");
        metaDataAttributes.add("LEFT_UPPER_LONGITUDE");
        metaDataAttributes.add("RIGHT_LOWER_LATITUDE");
        metaDataAttributes.add("RIGHT_LOWER_LONGITUDE");
        System.out.println("Setting metadata names..DONE");
        
        
        List<String> metaDataValues;
        
        System.out.println("Setting metadata values 1..");
        metaDataValues = setCoordinates("10","11","11","10");
        System.out.println("Setting metadata values 1..DONE. Pushing data 1..");
        disPro.pushPro(idClient, groupName, metaDataAttributes, metaDataValues, data, expirationTime, historyWindow, tag);
        System.out.println("Pushing data 1..DONE");
        
        System.out.println("Setting metadata values 2..");
        metaDataValues = setCoordinates("11","11","12","10");
        System.out.println("Setting metadata values 2..DONE. Pushing data 2..");
        disPro.pushPro(idClient, groupName, metaDataAttributes, metaDataValues, data, expirationTime, historyWindow, tag);
        System.out.println("Pushing data 2..DONE");
        
        System.out.println("Setting metadata values 3..");
        metaDataValues = setCoordinates("10","12","11","11");
        System.out.println("Setting metadata values 3..DONE. Pushing data 3..");
        disPro.pushPro(idClient, groupName, metaDataAttributes, metaDataValues, data, expirationTime, historyWindow, tag);
        System.out.println("Pushing data 3..DONE");
        
        System.out.println("Setting metadata values 4..");
        metaDataValues = setCoordinates("11","12","12","11");
        System.out.println("Setting metadata values 4..DONE. Pushing data 4..");
        disPro.pushPro(idClient, groupName, metaDataAttributes, metaDataValues, data, expirationTime, historyWindow, tag);
        System.out.println("Pushing data 4..DONE");
        
        System.out.println("Setting metadata values 5..");
        metaDataValues = setCoordinates("10","13","11","12");
        System.out.println("Setting metadata values 5..DONE. Pushing data 5..");
        disPro.pushPro(idClient, groupName, metaDataAttributes, metaDataValues, data, expirationTime, historyWindow, tag);
        System.out.println("Pushing data 5..DONE");
        
        System.out.println("Setting metadata values 6..");
        metaDataValues = setCoordinates("11","13","12","12");
        System.out.println("Setting metadata values 6..DONE. Pushing data 6..");
        disPro.pushPro(idClient, groupName, metaDataAttributes, metaDataValues, data, expirationTime, historyWindow, tag);
        System.out.println("Pushing data 6..DONE");
        
    }
    
    private List<String> setCoordinates(String LU_LAT, String LU_LON, String RL_LAT, String RL_LON) {
        List<String> coordinates = new ArrayList<String>();
        coordinates.add(LU_LAT);
        coordinates.add(LU_LON);
        coordinates.add(RL_LAT);
        coordinates.add(RL_LON);
        return coordinates;
    }
    
    
    public static void main(String[] args) {
        
        ClientA clientA = new ClientA();
        disPro = new DisServiceProProxy((short)0);
        disPro.init();
        disPro.registerDisServiceProProxyListener(clientA);
        disPro.registerMatchmakingLogListener(clientA);
        disPro.registerPeerStatusListener(clientA);
        
        (new Thread(clientA)).start();
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
        System.out.println("New peer: " + peerID);
        return true;
    }

    public boolean deadPeer(String peerID) {
        System.out.println("Dead peer: " + peerID);
        return true;
    }
}
