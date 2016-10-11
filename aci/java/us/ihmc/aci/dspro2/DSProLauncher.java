package us.ihmc.aci.dspro2;

/**
 * Initialize the C++ DSPro using JNI
 * @author Rita Lenzi (rlenzi@ihmc.us) - 3/31/2016
 */
public class DSProLauncher
{
    /**
     * Constructor
     * @param logDir directory containing the logs
     * @param configFile configuration file
     * @param metadataExtraAttrFile metadata extra attributes file
     * @param metadataValuesFile metadata extra values file
     * @param version identifies the version of dspro. It may be null
     * @throws DSProInitException if the node is is null
     */
    public DSProLauncher (String logDir, String configFile, String metadataExtraAttrFile, String metadataValuesFile,
                          String version) throws DSProInitException
    {
        int rc = initLoggers (logDir);
        if (rc != 0) {
            throw new DSProInitException ("Impossible to initialize the loggers - method failed with value " + rc);
        }

        rc = initConfigNative (configFile);
        if (rc != 0) {
            throw new DSProInitException ("Impossible to initialize the config manager - method failed with value " + rc);
        }

        String nodeId = getNodeId();
        if (nodeId == null) {
            throw new DSProInitException ("The DSPro node id cannot be null");
        }

        constructor (nodeId, version);
        rc = initDSProNative (metadataExtraAttrFile, metadataValuesFile);

        if (rc != 0) {
            throw new DSProInitException ("DSPro initialization failed with value " + rc);
        }
    }

    /**
     * Starts the <code>DSPro</code> main thread
     * @throws DSProInitException if any error occurs
     */
    public void start() throws DSProInitException
    {
        int rc = startNative();
        if (rc != 0) {
            throw new DSProInitException ("Start method on DSPro thread failed with return value " + rc);
        }
    }

    /**
     * Initialize the loggers in the specific directory
     * @param logDir directory containing the logs
     * @return 0 if the initialization is correct, a different value otherwise
     */
    private native int initLoggers (String logDir);

    /**
     * Initialize the config manager
     * @param configFile configuration file
     * @return 0 if the initialization is correct, a different value otherwise
     */
    private native int initConfigNative (String configFile);

    /**
     * Retrieves the node id assigned to the <code>DSPro</code> instance in the configuration file
     * @return the node id assigned to the <code>DSPro</code> instance in the configuration file
     */
    private native String getNodeId();

    /**
     * Native method that builds an object <code>DSPro</code> (constructor equivalent).
     * @param nodeId uniquely identifies the node in the network, it cannot be null
     * @param version identifies the version of dspro. It may be null
     */
    private native void constructor (String nodeId, String version);

    /**
     * Initialize <code>DSPro</code> reading the configuration properties and the extra attributes from the files
     * @param metadataExtraAttrFile metadata extra attributes file
     * @param metadataValuesFile metadata extra values file
     * @return 0 if the initialization is correct, a different value otherwise
     */
    private native int initDSProNative (String metadataExtraAttrFile, String metadataValuesFile);

    /**
     * Starts the <code>DSPro</code> main thread
     * @return 0 if everything is correct, a different value otherwise
     */
    private native int startNative();

    /**
     * Handles exceptions related to <code>DSPro</code> initialization
     */
    public class DSProInitException extends Exception
    {
        /**
         * Constructs a <code>FatalFederationException</code> with the given message
         * @param msg description of exception
         */
        public DSProInitException (String msg)
        {
            super (msg);
        }
    }



    private long _dspro;
    private long _configManager;

    static {
        System.loadLibrary ("dsprojniwrapper");
    }
}
