package us.ihmc.aci.agserve;

import java.net.URL;
import java.util.Hashtable;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.Dime;
import us.ihmc.util.HTTPHelper;
import us.ihmc.util.IDGenerator;

/**
 * AgServeAdminClient
 *
 * Provides AgServe related administrative functions
 *
 * Created on August 27, 2007, 3:52 PM
 *
 * @author nsuri
 * @author mrebeschini
 */
public class HTTPAgServeAdminClient extends HTTPAgServeClient
{
    public HTTPAgServeAdminClient()
    {
        super();
    }

    public HTTPAgServeAdminClient (boolean useMockets)
    {
        super(useMockets);
    }

    public HTTPAgServeAdminClient (String kernelAddress)
    {
        init (kernelAddress, DEFAULT_ACI_KERNEL_PORT);
        _clientUUID = IDGenerator.generateID();
    }

    public HTTPAgServeAdminClient (String kernelAddress, int kernelPort)
    {
        init (kernelAddress, kernelPort);
        _clientUUID = IDGenerator.generateID();
    }

    /**
     * Activates the specified service on the specified node
     *
     * @param nodeUUID the UUID of the node on which the service should be activated
     * @param serviceName the name of the service to be activated
     *
     * @return uri of the activated service (format is "acil://[<_instanceUUID>]|[<_nodeUUID>]|[<ipaddr[:_port]>]")
     */
    public String activateServiceRemotely (String nodeUUID, String serviceName)
        throws Exception
    {
        if (nodeUUID == null) {
            throw new NullPointerException ("nodeUUID");
        }
        CommHelper chRemoteNode = connect (nodeUUID, false);
        String serviceInstanceUUID = null;
        try {
            serviceInstanceUUID = activateService (chRemoteNode, nodeUUID, serviceName, true);
        }
        catch (CommException ex) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            chRemoteNode = connect (nodeUUID, true);
            serviceInstanceUUID = activateService (chRemoteNode, nodeUUID, serviceName, true);
        }
        return "acil://" + serviceInstanceUUID + "|" + nodeUUID + "|";
    }

    public boolean deactivateServiceRemotely (String nodeUUID, String serviceInstanceUUID)
        throws Exception
    {
        if (nodeUUID == null) {
            throw new NullPointerException ("nodeUUID");
        }
        CommHelper chRemoteNode = connect (nodeUUID, false);

        try {
            return deactivateService (chRemoteNode, nodeUUID, serviceInstanceUUID);
        }
        catch (CommException ex) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            chRemoteNode = connect (nodeUUID, true);
            return deactivateService (chRemoteNode, nodeUUID, serviceInstanceUUID);
        }
    }

    public void deployServiceRemotely (String nodeUUID, URL acrFileURL)
        throws Exception
    {
        CommHelper ce = connect (nodeUUID, false);
        try {
            deployService (ce, acrFileURL);
        }
        catch (CommException comex) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            connect (nodeUUID, false);
            deployService (ce, acrFileURL);
        }
    }

    /**
     * Relocates service serviceInstanceUUID to destNodeUUID. The service state
     * is captured and restored in the remote node.
     *
     * @param serviceInstanceUUID
     * @param destNodeUUID
     */
    public void relocateService (String serviceInstanceUUID, String destNodeUUID)
        throws Exception
    {
        connect (false);
        try {
            relocateService (_commHelper, serviceInstanceUUID, destNodeUUID);
        }
        catch (CommException ex) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            connect (true);
            relocateService (_commHelper, serviceInstanceUUID, destNodeUUID);
        }
    }

    public void relocateAllServices (String destNodeUUID)
        throws Exception
    {
        connect (false);

        try {
            relocateAllServices (_commHelper, destNodeUUID);
        }
        catch (CommException ex) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            connect (true);
            relocateAllServices (_commHelper, destNodeUUID);
        }
    }

    public void restoreService (String nodeUUID, String instanceUUID, String serviceName, byte[] stateBuf, String vmContainerName)
        throws Exception
    {
        CommHelper ce = connect (nodeUUID, false);
        try {
            restoreService (ce, instanceUUID, serviceName, stateBuf, vmContainerName);

        }
        catch (CommException e) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            ce = connect (nodeUUID, true);
            restoreService (ce, instanceUUID, serviceName, stateBuf, vmContainerName);
        }
    }

/*
    public float getResourceInfo (String nodeUUID, int queryType, String serviceName, String methodSignature)
        throws Exception
    {
        CommHelper ce = connect (nodeUUID, false);
        try {
            return getResourceInfo (ce, queryType, serviceName, methodSignature);
        }
        catch (CommException e) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            ce = connect (nodeUUID, true);
            return getResourceInfo (ce, queryType, serviceName, methodSignature);
        }
    }
*/

    /**
     * Query the resource utilization database on the specified node
     *
     * @param nodeUUID the UUID of the node on which the service should be activated
     *
     * @return the resource data
     */
    public float getResourceInfoRemotely (String nodeUUID, int queryType, String serviceName, String methodSignature)
        throws Exception
    {
        if (nodeUUID == null) {
            throw new NullPointerException ("nodeUUID");
        }
        CommHelper chRemoteNode = connect (nodeUUID, false);
        float resourceInfo = 0;
        try {
            resourceInfo = getResourceInfo (queryType, serviceName, methodSignature);
        }
        catch (CommException ex) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            chRemoteNode = connect (nodeUUID, true);
            resourceInfo = getResourceInfo (queryType, serviceName, methodSignature);
        }
        return resourceInfo;
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS //////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    private static void restoreService (CommHelper commHelper, String instanceUUID, String serviceName, byte[] stateBuf, String vmContainerName)
        throws Exception
    {
        Dime dimeMsg = new Dime();
        dimeMsg.addRecord ("serviceState", stateBuf, serviceName);

        HTTPHelper httpHelper = new HTTPHelper (commHelper);
        Hashtable htAux = new Hashtable();
        htAux.put ("Action", "relocate");
        htAux.put ("Action-Type", "restore");
        htAux.put ("VMContainerName", vmContainerName);
        htAux.put ("Content-Type", "dime");
        htAux.put ("Content-Length", Integer.toString(dimeMsg.getDimeLength()));
        htAux.put ("Expect", "100-continue");

        httpHelper.sendPOSTRequest (instanceUUID, htAux);
        httpHelper.expectResponse (100);

        commHelper.sendBlob (dimeMsg.getDime());
        httpHelper.expectResponse (200);
        commHelper.closeConnection();
    }

    private static void relocateService (CommHelper commHelper, String serviceInstanceUUID, String destNodeUUID)
        throws Exception
    {
        HTTPHelper httpHelper = new HTTPHelper (commHelper);
        Hashtable htAux = new Hashtable();
        htAux.put ("Action", "relocate");
        htAux.put ("Action-Type", "migrate");
        htAux.put ("DestNodeUUID", destNodeUUID);
        htAux.put ("VMContainerName", "JikesVM");
        htAux.put ("Content-Type", "dime");
        htAux.put ("Content-Length", "0");

        httpHelper.sendPOSTRequest (serviceInstanceUUID, htAux);

        // expect a "HTTP/1.1 200 OK"
        httpHelper.expectResponse (200);
    }

    private static void relocateAllServices (CommHelper commHelper, String destNodeUUID)
        throws Exception
    {
        HTTPHelper httpHelper = new HTTPHelper (commHelper);
        Hashtable htAux = new Hashtable();
        htAux.put ("Action", "relocate");
        htAux.put ("Action-Type", "migrateAll");
        htAux.put ("DestNodeUUID", destNodeUUID);
        htAux.put ("VMContainerName", "JikesVM");
        htAux.put ("Content-Type", "dime");
        htAux.put ("Content-Length", "0");

        httpHelper.sendPOSTRequest (destNodeUUID, htAux);

        // expect a "HTTP/1.1 200 OK"
        httpHelper.expectResponse (200);
    }
}

