package us.ihmc.aci.coord;

import us.ihmc.aci.grpMgrOld.GroupManager;
import us.ihmc.aci.grpMgrOld.GroupManagerException;
import us.ihmc.aci.grpMgrOld.GroupManagerListener;
import us.ihmc.aci.grpMgrOld.GroupManagerWrapper;
import us.ihmc.util.ConfigLoader;
import us.ihmc.util.NSLookup;
import us.ihmc.util.ByteArray;

import java.io.File;
import java.util.Vector;
import java.util.StringTokenizer;
import java.net.InetAddress;


/**
 * Coordinator.java
 *
 * Created on January 26, 2006, 2:56 PM
 */
public class Coordinator implements GroupManagerListener
{
    public Coordinator()
        throws Exception
    {
        try {
            String nomadsHome = (String) System.getProperties().get ("nomads.home");
            String cfgFilePath = File.separator + "aci" + File.separator + "conf" + File.separator + "grpMgr.properties";
            _cfgLoader = ConfigLoader.initDefaultConfigLoader (nomadsHome, cfgFilePath);
            
            if (_cfgLoader.hasProperty ("GroupManagerPortNum")) {
                _portNumber = _cfgLoader.getPropertyAsInt ("GroupManagerPortNum");
            }
            //System.out.println("PORT: " + _portNumber);

            // Initialize the Group Manager
            if (_cfgLoader.hasProperty ("GroupManagerNetIFs")) {
                Vector ipAddrs = new Vector(); //Vector<InetAddress>
                StringTokenizer st = new StringTokenizer (_cfgLoader.getProperty ("GroupManagerNetIFs")," :;");
                while (st.hasMoreTokens()) {
                    ipAddrs.addElement (InetAddress.getByName (st.nextToken()));
                }
                try {
                    _gm = new GroupManager (_portNumber, ipAddrs);
                } 
                catch (GroupManagerException gme) {
                    System.out.println ("Error! Cannot initialize Group Manager");
                    return;
                }
            }
            else {
                try {
                    _gm = new GroupManager (_portNumber);
                }
                catch (Exception e) {
                    System.out.println ("Error! Cannot initialize Group Manager: " + e);
                    return;
                }
            }
            
            // Configure the GroupManager 
            if (_cfgLoader.hasProperty ("GroupManagerPingInterval")) {
                _gm.setPingInterval (_cfgLoader.getPropertyAsInt ("GroupManagerPingInterval"));
            }

            if (_cfgLoader.hasProperty ("GroupManagerInfoBCastInterval")) {
                _gm.setInfoBCastInterval (_cfgLoader.getPropertyAsInt ("GroupManagerInfoBCastInterval"));
            }

            if (_cfgLoader.hasProperty ("GroupManagerPingHopCount")) {
                _gm.setPingHopCount (_cfgLoader.getPropertyAsShort ("GroupManagerPingHopCount"));
            }

            if (_cfgLoader.hasProperty ("GroupManagerPeerSearchHopCount")) {
                _gm.setPeerSearchHopCount (_cfgLoader.getPropertyAsShort ("GroupManagerPeerSearchHopCount"));
            }

            if (_cfgLoader.hasProperty ("GroupManagerDriverIdentity")) {
                _searcher = _cfgLoader.getProperty ("GroupManagerDriverIdentity").equalsIgnoreCase ("searcher");
                _replier = _cfgLoader.getProperty ("GroupManagerDriverIdentity").equalsIgnoreCase ("replier");
            }
            
            _gm.setNodeName ("Coordinator - " + NSLookup.getHostname());

            _gmw = new GroupManagerWrapper (_gm);
            _gmw.setListener (this);
        }
        catch (GroupManagerException e) {
            System.out.println ("Exception in Coordinator.init(): " + e);
            throw e;
        }
    }        

    public void init()
    {
        try {            
            String str = "init_param";
            
            _gm.createPublicPeerGroup ("ihmc_public", str.getBytes()); 
            _gm.createPrivatePeerGroup ("ihmc_private", "nomads", str.getBytes());
            
            _gm.start();
            _gmw.start();
            
            if (_searcher) {
                String peerSearchParam = "Hello World!";
                int peerSearchInterval = DEFAULT_PEER_SEARCH_INTERVAL;
                if (_cfgLoader.hasProperty ("GroupManagerDriverPeerSearchParam")) {
                    peerSearchParam = _cfgLoader.getProperty ("GroupManagerDriverPeerSearchParam");
                }
                if (_cfgLoader.hasProperty ("GroupManagerDriverPeerSearchInterval")) {
                    peerSearchInterval = _cfgLoader.getPropertyAsInt ("GroupManagerDriverPeerSearchInterval");
                }

                int i = 0;
                while (!_gm.hasTerminated() && !_gm.hasTerminated()) {
                    Thread.sleep (peerSearchInterval);
                    String searchParam = peerSearchParam + "_f" + peerSearchInterval + "_#" + ++i + '\0';
                    _gm.startPeerSearch ("ihmc_public", searchParam.getBytes());
                    _gm.startPeerSearch ("ihmc_private", searchParam.getBytes());
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("peer search request received - groupName: " + groupName + ", nodeUUID: " +
                            nodeUUID + ", searchUUID: " + searchUUID + " - param: " + new String (param));
    }
    
    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("peer search result received - groupName: " + groupName + ", nodeUUID: " +
                            nodeUUID + ", searchUUID: " + searchUUID + " - param: " + new String (param));
    }

    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data)
    {
    }

    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String searchUUI)
    {
    }

    public void conflictWithPrivatePeerGroup (String groupName, String nodeUUID)
    {
        System.out.println ("Conflicting group " + groupName + " on peer " + nodeUUID);
    }
    
    public void deadPeer (String nodeUUID)
    {
        System.out.println ("Peer - " + nodeUUID + " died");
    }

    public void groupListChange (String nodeUUID)
    {
        System.out.println ("Peer - " + nodeUUID + " changed it's group list");
    }

    public void groupMemberLeft (String groupName, String memberUUID)
    {
        System.out.println ("Group Member " + memberUUID + " left group " + groupName);
    }

    public void newGroupMember (String groupName, String memberUUID, byte[] joinData)
    {
        System.out.println ("New Group Member in group " + groupName + ": " + memberUUID);
    }

    public void newPeer (String nodeUUID)
    {
        System.out.println ("Detected new peer - " + nodeUUID);
    }
    
    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] param)
    {
    	System.out.println ("\nGroupName: " + groupName + "\tnodeUUID: " + nodeUUID);

    	int idx = 0;
    	while (idx < param.length) {
    		byte paramType = param[idx++];
    		if (paramType == 0x00) break;
    		
    		byte paramLength = param[idx++];
    		
    		//in this case we already know that all the parameters are shorts
    		short paramContents = ByteArray.byteArrayToShort (param, idx);
    		idx+=paramLength;
    		
    		System.out.println ("paramType = " + paramType + 
    				"\tparamLength = " + paramLength + "\tparamContents = " + 
    				paramContents);
    	}
    }
    
    public static void main (String[] args)
    {
        try {
            Coordinator coordinator = new Coordinator();
            coordinator.init();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static final int DEFAULT_GROUP_MANAGER_PORT_NUM = 2007;
    private static final int DEFAULT_PEER_SEARCH_INTERVAL = 5000;
    private boolean _searcher = false;
    private boolean _replier = false;
    private ConfigLoader _cfgLoader;
    private GroupManager _gm;
    private GroupManagerWrapper _gmw;
    private int _portNumber = DEFAULT_GROUP_MANAGER_PORT_NUM;
}
