package us.ihmc.aci.kernel;

import java.util.Vector;

/**
 * KernelProxy.java
 *
 * This class provides methods that can be invoked by priveleged Java code
 * (such as a Coordinator) running inside a VM Container.
 *
 * Created on July 25, 2007, 1:37 PM
 *
 * @author nsuri
 */
public class KernelProxy
{
    public KernelProxy()
    {
    }

    /*
     * Returns the path to the config file currently being used by the kernel
     *
     * @return path to the config file
     */
    public native String getConfigFilePath();
    
    /*
     * Returns the properties that have been defined for this node, which are
     * a comma-separated list of attributes or attribute=value pairs
     * 
     * @return the properties for this node or null if no properties have been set
     */
    public native String getNodeProperties();
    
    /* Method to query the resource utilization database.
     * 
     * @flagQueryType type of query.
     * 
     */
    public native float getResourceInfo (int flagQueryType, String serviceName, String methodSignature);

    public native String locateServiceInstance (String instanceUUID);
    public native Vector getNetworkTopology();
    public static native int getDefaultServiceContainerType();

    public String activateService (String serviceName)
        throws Exception
    {
        return this.activateService (serviceName, getDefaultServiceContainerType());
    }

    public native String activateService (String serviceName, int containerType)
        throws Exception;

    public native void deactivateService (String instanceUUID)
        throws Exception;

    public native byte[] captureService (String instanceUUID)
        throws Exception;

    public native void migrateService (String instanceUUID, String destNodeUUID)
        throws Exception;

    public native void migrateAllServices (String destNodeUUID)
        throws Exception;

    // Service Container Types
    public static final int SCT_Undefined = 0x00;
    public static final int SCT_Aroma = 0x01;
    public static final int SCT_Java = 0x02;
    public static final int SCT_Jikes = 0x03;
}

