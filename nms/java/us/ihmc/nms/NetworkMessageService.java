package us.ihmc.nms;

import java.io.IOException;

/**
 * Initialize the C++ Network Message Service using JNI
 * @author Rita Lenzi (rlenzi@ihmc.us) - 2/5/2016
 */
public class NetworkMessageService
{
    /**
     * Constructor
     */
    public NetworkMessageService()
    {
        constructor();
    }

    /**
     *Initialize the NetworkMessageService reading the configuration properties from the file
     * @param configFile configuration file
     * @throws IOException if the initialization is not correct
     */
    public void init (String configFile) throws IOException
    {
        int rc = initNativeConfig (configFile);

        if (rc != 0) {
            throw new IOException ("Unable to initializing NetworkMessageService, result code is: " + rc);
        }
    }

    /**
     * Native method that builds a <code>NetworkMessageService</code> object (equivalent to the constructor)
     */
    private native void constructor();

    /**
     * Initialize the NetworkMessageService reading the configuration properties from the file
     * @param configFile configuration file
     * @return 0 if the initialization is correct, a different value otherwise
     */
    private native int initNativeConfig (String configFile);

    /**
     * Starts the NetworkMessageService main thread
     */
    public native void start();

    private long _networkMessageService;

    static {
        System.loadLibrary ("nmsjavawrapper");
    }
}
