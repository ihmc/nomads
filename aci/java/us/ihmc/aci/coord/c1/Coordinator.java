package us.ihmc.aci.coord.c1;

import us.ihmc.aci.grpMgrOld.*;
import us.ihmc.util.ConfigLoader;
import us.ihmc.net.NetUtils;
import us.ihmc.io.LineReaderInputStream;
import us.ihmc.util.Dime;
import us.ihmc.util.DimeRecord;
import us.ihmc.aci.coord.NodeInfo;

import java.util.*;
import java.net.*;
import java.io.*;

/**
 * Coordinator.java
 *
 * Created on September 28, 2006, 9:54 PM
 *
 * @author  nsuri
 */
public class Coordinator
{
    public Coordinator()
    {
        // Initialize the ConfigLoader
        String nomadsHome = (String) System.getProperties().get ("nomads.home");
        String cfgFilePath = File.separator + "aci" + File.separator + "conf" + File.separator + "aci.properties";
        _cfgLoader = ConfigLoader.initDefaultConfigLoader (nomadsHome, cfgFilePath);

        try {
            if (_cfgLoader.hasProperty ("aci.coordinator.name")) {
                _cfgLoader.setProperty ("aci.coordinator.name", COORDINATOR_NAME);
            }
        
            if (_cfgLoader.hasProperty ("aci.coordinator.port")) {
                _cfgLoader.setProperty ("aci.coordinator.port", DEFAULT_COORDINATOR_PORT_NUM + "");
            }

            if (_cfgLoader.hasProperty ("aci.groupmanager.default.groupname")) {
                _cfgLoader.setProperty ("aci.groupmanager.default.groupname", DEFAULT_GROUP_MANAGER_GROUP_NAME);
            }

            if (_cfgLoader.hasProperty ("aci.kernel.port")) {
                _cfgLoader.setProperty ("aci.kernel.port", DEFAULT_KERNEL_PORT_NUM + "");
            }   

            if (_cfgLoader.hasProperty ("aci.groupmanager.port")) {
                _cfgLoader.setProperty ("aci.groupmanager.port", DEFAULT_GROUP_MANAGER_PORT_NUM + "");
            }
       
            // Initializing the GroupManager and the NodeStateMonitor 
            _httpHandler = new HTTPProtocolHandler (this, _cfgLoader.getPropertyAsInt ("aci.coordinator.port"));
            _gm = new GroupManager (_cfgLoader.getPropertyAsInt ("aci.groupmanager.port"));
            _gm.setNodeName (_cfgLoader.getProperty ("aci.coordinator.name"));
            _nsm = new NodeStateMonitor (_cfgLoader.getProperty ("aci.groupmanager.default.groupname"));
            _gm.setListener (_nsm);
            //_gm.createPrivatePeerGroup (_cfgLoader.getProperty ("aci.groupmanager.default.groupname"), "nomads", null);
            _gm.createPublicPeerGroup (_cfgLoader.getProperty ("aci.groupmanager.default.groupname"), null);
            _gm.start();
            _httpHandler.start();
        }
        catch (Exception e) {
            System.out.println ("Exception in Coordinator constructor: " + e);
        }
    }

    public class ActivationInfo
    {
        public String nodeUUID;
        public String instanceUUID;
    }

    /**
     * Selects a node, activates the service on the node, and returns the UUID of the
     * node and the UUID of the service instance.
     * 
     * @param serviceName         the classname of the service
     * @param clientNodeUUID      the UUID of the node requesting the service activation
     *
     * @return     An instance of ActivationInfo with the UUID of the node where the
     *             service has been activated and the UUID of the service instance or
     *             null in case of error
     */
    public ActivationInfo activateService (String serviceName, String clientNodeUUID)
    {
        System.out.println ("Coordinator.activateService: Received activation for service: " + serviceName + 
                            " from clientNode: " + clientNodeUUID);
        
        String lowestCPUNodeUUID = getLowestCPUNode();

        System.out.println ("Coordinator.activateService - elected node for service " + serviceName +
                            " is: " + lowestCPUNodeUUID);
        
        String instanceUUID = activateRemoteService (serviceName, lowestCPUNodeUUID);
       
        ActivationInfo activationInfo = null;
        if (instanceUUID != null) {
            activationInfo = new ActivationInfo();
            activationInfo.nodeUUID = lowestCPUNodeUUID;
            activationInfo.instanceUUID = instanceUUID;
        }
            
        return activationInfo;
    }

    private String getLowestCPUNode()
    {
        String nodeUUID = null;
        int minCpuVal = 100;
        Iterator it = _nsm.getActiveNodes();
        while (it.hasNext()) {
            String uuid = (String) it.next();
            NodeInfo nInfo = (NodeInfo) _nsm.getNodeInfo (uuid);
            if (nInfo.cpuUtilizationOther < minCpuVal) {
                minCpuVal = nInfo.cpuUtilizationOther;
                nodeUUID = uuid;
            }
        }

        return nodeUUID;
    }

    private String activateRemoteService (String serviceName, String clientNodeUUID)
    {   
        PeerInfo pgmi = _gm.getPeerInfo (clientNodeUUID);
        Vector netIFs = pgmi.nicsInfo; //Vector<NICInfo>
        InetAddress nodeAddr = NetUtils.determineDestIPAddr (pgmi.nicsInfo, _gm.getActiveNICsInfo());

        try {
            Socket s = new Socket (nodeAddr, _cfgLoader.getPropertyAsInt ("aci.kernel.port"));

            LineReaderInputStream lris = new LineReaderInputStream (s.getInputStream());
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter (s.getOutputStream()));
	        bw.write("POST " + serviceName + " HTTP/1.1\r\n");
            bw.write("Action: activate_locally\r\n");
            bw.write("Content-Type: dime\r\n");
            bw.write("Content-Length: 0\r\n");
            bw.write("Expect: 100-continue\r\n");
            bw.write("\r\n");
            bw.flush();

            String lineAux = lris.readLine();

            StringTokenizer st = new StringTokenizer(lineAux, "\n\r:,; ");
            st.nextToken();
            String respCode = st.nextToken();

            if (!"100".equals (respCode)) {
                throw new Exception("Error occurred: " + respCode);
            }

            // send the DIME here (not necessary right now for 'activate')
            bw.write("\r\n");
            bw.flush();

            // expect a "HTTP/1.1 200 OK"
            lineAux = lris.readLine();
            st = new StringTokenizer(lineAux, "\n\r:,; ");
            st.nextToken();
            respCode = st.nextToken();

            if (!"200".equals(respCode)) {
                throw new Exception("Error occurred: " + respCode);
            }

            // expect "Content-Type: xml+soap"
            //        "Content-Length: <dimeLength>
            String contentType = null;
            int contentLength = 0;

            while ( (lineAux = lris.readLine()) != null ) {
                if (lineAux.equals("\r\n") || lineAux.equals("")) {
                    break;
                }
                st = new StringTokenizer(lineAux, "\n\r;:, ");

                String key = st.nextToken();
                String value = st.nextToken();

                if (key.equals("Content-Type")) {
                    contentType = value;
                }
                else if (key.equals("Content-Length")) {
                    contentLength = Integer.parseInt(value);
                }
            }

            //now read and parse the dime
            /*
            Dime dimeMsg = new Dime(lris);
            if (dimeMsg.getRecordCount() < 1) {
                throw new Exception("the DIME message received seems to be empty or corrupted");
            }

            DimeRecord dimeRec = (DimeRecord) (dimeMsg.getRecords().nextElement());

            //this should be the SOAP message (eventually it will be).
            // for now, just expect something like:
            // <serviceInstanceUUID>1234-1234-1234-1234</serviceInstanceUUID>
            */
            lineAux = new String(lris.readLine());
            String uuid = lineAux.replaceAll("<[^>]+>", "");
            return uuid;
        }
        catch (Exception e) {
            e.printStackTrace();
            return null;
        }        
    }

    public static void main (String[] args)
    {
        if (System.getProperties().get ("nomads.home") == null) {
            System.out.println ("Error! You need to define nomads.home");
            System.exit (-1);
        }
        
        Coordinator c = new Coordinator();
    }

   
    public static final int DEFAULT_KERNEL_PORT_NUM = 2005; 
    public static final int DEFAULT_GROUP_MANAGER_PORT_NUM = 2007;
    public static final int DEFAULT_COORDINATOR_PORT_NUM = 2009;
    public static final String DEFAULT_GROUP_MANAGER_GROUP_NAME = "ihmc.aci";
    public static final String COORDINATOR_NAME = "ACICentralCoordinator";

    private ConfigLoader _cfgLoader;    
    private HTTPProtocolHandler _httpHandler;
    private GroupManager _gm;
    private NodeStateMonitor _nsm;
}
