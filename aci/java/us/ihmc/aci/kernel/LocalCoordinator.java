package us.ihmc.aci.kernel;

import us.ihmc.aci.kernel.ServiceRequest;

/**
 *
 * @author mrebeschini
 */
public interface LocalCoordinator
{
    /**
     * Called to initialize the LocalCoordinator
     *
     * @param serviceManager the Service Manager object
     * @param kernelProxy the Kernel Proxy object
     */
    public void init (ServiceManager serviceManager, KernelProxy kernelProxy);

    /**
     * Request that a service instance be activated
     *
     * @param requestorURI the ACI Locator URI of the requestor
     * @param serviceName the name of the service to be activated
     * @param onLocalNodeOnly if true then the request is to activate the service on the local node only
     *
     * @return ACI Locator URI (format is acil://[<instanceUUID>]|[<nodeUUID>]|[<ipaddr[:port]>]) where the
     * ipaddr and port field may be blank and the nodeUUID may be blank if the service is activated on the
     * local node or null in case of error
     */
    public String activateServiceRequest (String requestorURI, String serviceName, boolean onLocalNodeOnly) throws Exception;

    /**
     * Deactivate a service instance
     *
     * @param requestor the ACI Locator URI of the requestor
     * @param serviceInstanceURI the ACI Locator URI for the service
     *
     * @return true if the service has been deactivated successfully or false otherwise
     */
    public boolean deactivateServiceInstanceRequest (String requestorURI, String serviceInstanceURI) throws Exception;

    /**
     * Lookup the current location of a service instance
     *
     * @param requestorURI the ACI Locator URI of the requestor
     * @param instanceUUID the instance UUID of the service
     *
     * @return ACI Locator URI (format is acil://[<instanceUUID>]|[<nodeUUID>]|[<ipaddr[:port]>]) for the
     * service instance or null if not found
     */
    public String lookupServiceInstanceRequest (String requestorURI, String instanceUUID, String oldNodeUUID) throws Exception;

    /**
     * Check to see whether a service can be restored onto this node
     * 
     * @param requestorURI the ACI Locator URI of the node originating the request
     * @param sourceURI the ACI Locator URI of the node from which the service will be restored
     * @param instanceUUID the UUID of the service instance being restored
     * 
     * @return true if the restore operation should be allowed or false otherwise
     */
    public boolean allowRestoreServiceInstance (String requestorURI, String sourceURI, String instanceUUID) throws Exception;

    /**
     * Inform the coordinator that a service has been restored after migration
     *
     * @param serviceName the name of the service
     * @param instanceUUID the instance UUID of the service
     */
    public void serviceRestored (String serviceName, String instanceUUID);

    /**
     * This method is called when the ACI Kernel is about to be
     * suspended / hybernated / terminated
     */
    public void kernelSuspending() throws Exception;

    /**
     * This method is called when the ACI Kernel resumes
     * from a suspend / hybernation
     */
    public void kernelResumed() throws Exception;
    
    /**
     * This method is called when the ACI Kernel has
     * updated information about the local node
     *
     * @param the information for the local node in the NodeMonInfo format
     * that can be parsed by NodeInfo.
     */
    public void nodeInfoUpdated (byte[] nodeInfoData) throws Exception;
}
