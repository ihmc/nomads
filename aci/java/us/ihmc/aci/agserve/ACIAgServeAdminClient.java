package us.ihmc.aci.agserve;

import java.net.URL;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;

/**
 * 
 * @author mrebeschini
 */
public class ACIAgServeAdminClient extends ACIAgServeClient
{
    public ACIAgServeAdminClient()
    {
    }

    public ACIAgServeAdminClient (String kernelAddress)
    {
        init (kernelAddress, DEFAULT_ACI_KERNEL_PORT);
    }
    
    public ACIAgServeAdminClient (String kernelAddress, int kernelPort)
    {
    	init (kernelAddress, kernelPort);
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
    // PROTECTED METHODS ///////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
        
    protected void restoreService (CommHelper commHelper, String instanceUUID, String serviceName, byte[] stateBuf, String vmContainerName)
        throws Exception
    {
		commHelper.sendLine ("SPEAK relocate/1.0");
		
		commHelper.sendLine ("START_RELOCATEINFO");
		commHelper.sendLine ("ActionType restore");
		commHelper.sendLine ("ServiceName " + serviceName);
		commHelper.sendLine ("ServiceInstanceUUID " + instanceUUID);
		commHelper.sendLine ("StateSize " + stateBuf.length);
        if (vmContainerName != null) {
		    commHelper.sendLine ("VMContainerName " + vmContainerName);
        }
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
		commHelper.sendLine ("END_RELOCATEINFO");
		
		commHelper.receiveMatch ("RELOCATEINFO_VALID");
		
		commHelper.sendLine ("START_RELOCATESERVICE");
		commHelper.sendBlob (stateBuf);
		commHelper.sendLine ("END_RELOCATESERVICE");
		
		commHelper.receiveMatch ("RELOCATESERVICE_OK");
    }
    
    protected void relocateService (CommHelper commHelper, String serviceInstanceUUID, String destNodeUUID)
    	throws Exception
    {
		commHelper.sendLine ("SPEAK relocate/1.0");
		
		commHelper.sendLine ("START_RELOCATEINFO");
		commHelper.sendLine ("ActionType migrate");
		commHelper.sendLine ("ServiceInstanceUUID " + serviceInstanceUUID);
		commHelper.sendLine ("DestNodeUUID " + destNodeUUID);
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
		commHelper.sendLine ("END_RELOCATEINFO");
		
		commHelper.receiveMatch ("RELOCATEINFO_VALID");
		commHelper.receiveMatch ("RELOCATESERVICE_OK");
    }
   
    protected void relocateAllServices (CommHelper commHelper, String destNodeUUID)
        throws Exception
    {
		commHelper.sendLine ("SPEAK relocate/1.0");
		
		commHelper.sendLine ("START_RELOCATEINFO");
		commHelper.sendLine ("ActionType migrateAll");
		commHelper.sendLine ("DestNodeUUID " + destNodeUUID);
		commHelper.sendLine ("RequestorUUID " + _clientUUID);
		commHelper.sendLine ("END_RELOCATEINFO");
		
		commHelper.receiveMatch ("RELOCATEINFO_VALID");
		commHelper.receiveMatch ("RELOCATESERVICE_OK");
    }
}

