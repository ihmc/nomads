/**
 * AgserveClient
 *
 * @author Marco Arguedas      <marguedas@ihmc.us>
 * @author Matteo Rebeschini   <mrebeschini@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.agserve;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.URL;
import java.util.Hashtable;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.mockets.StreamMocket;
import us.ihmc.util.Dime;
import us.ihmc.util.IDGenerator;

/**
 *
 */
public abstract class AgServeClient
{
    public AgServeClient()
    {
        this (false);
    }

    public AgServeClient (boolean useMockets)
    {
        init ("127.0.0.1", DEFAULT_ACI_KERNEL_PORT, useMockets);
        _clientUUID = IDGenerator.generateID();
        _useMockets = useMockets;
    }

    /**
     *
     */
    public void setClientUUID (String clientUUID)
    {
        _clientUUID = clientUUID;
    } //setClientUUID()

    /**
     *
     */
    public void setTimeout (int timeout)
    {
        _timeout = timeout;

        if (_commHelper != null) {
            if (_useMockets) {
                //TODO: set timeout in mockets... somehow...
            }
            else {
                Socket sock = _commHelper.getSocket();
                if (sock != null) {
                    try {
                        sock.setSoTimeout (timeout);
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
        }
    } //setTimeout()

    /**
     * Checks whether this instance of the AgServerClient is currently connected
     * to the local ACIKernel. If it is not connected, it will try and connect.
     */
    public void connect (boolean forceReconnect) throws IOException
    {
        if (forceReconnect) {
            try {
                this.disconnect();
            }
            catch (Exception ex) {
                // intentionally left blank.
            }

            _commHelper = null;
        }

        if (_commHelper == null) {
            _commHelper = connect (_aciKernelAddress, _aciKernelPort);
            if (! authenticate(_commHelper)) {
                throw new IOException ("authentication failed");
            }
        }
    } //connect();

    /**
     *
     */
    protected CommHelper connect (String kernelAddress, int kernelPort) throws IOException
    {
        if (_useMockets) {
            return mocketConnect(kernelAddress, kernelPort);
        }
        else {
            return socketConnect(kernelAddress, kernelPort);
        }
    } //connect()

    /**
     *
     */
    public void disconnect() throws IOException
    {
        if (_commHelper != null) {
            _commHelper.closeConnection();
            _commHelper = null;
        }
    } //disconnect()

    /**
     *
     */
    public String getKernelAddress()
    {
        return _aciKernelAddress;
    } //getKernelAddress()

    /**
     *
     */
    public int getKernelPort()
    {
        return _aciKernelPort;
    } //getKernelPort()

    /**
     *
     */
    public String activateService (String serviceName) throws Exception
    {
        try {
            connect (false);
            return activateService (_commHelper, null, serviceName, false);
        }
        catch (CommException ex) {
            log ("Error communicating with the ACIKernel. Will try to reconnect");
            connect (true);
            return activateService (_commHelper, null, serviceName, false);
        }
    } //activateService()

    /**
     *
     */
    public Object invokeService (String instanceUUID, String methodName, Object[] params, boolean asynchronous)
        throws Exception
    {
        Dime request = _agServeUtils.encodeBinaryRequestParams (params);
        Dime reply = invokeService (instanceUUID, methodName, request, asynchronous);
        if (reply != null) {
            return _agServeUtils.decodeObjectFromDime (reply);
        }
        return null;
    }

    public Dime invokeService (String instanceUUID, String methodName, Dime dimeRequest, boolean asynchronous)
        throws Exception
    {
        String nodeUUID = (String) _svcLocations.get (instanceUUID);
        try {
            return invokeService (nodeUUID,
                                  instanceUUID,
                                  methodName,
                                  dimeRequest,
                                  asynchronous);
        }
        catch (Exception ex) {
            //DEBUG
            ex.printStackTrace();
            String exMsg = ex.getMessage();
            if ((exMsg.indexOf("404") >= 0) || (exMsg.indexOf ("410") >= 0)) { // Not Found OR Gone
                nodeUUID = lookupService (instanceUUID, nodeUUID);
                if (nodeUUID == null) {
                    ex = new Exception ("Could not locate nodeUUID for service instance");
                    throw ex;
                }
                else {
                    log ("service " + instanceUUID + " has been relocated to node " + nodeUUID + ".");
                    _svcLocations.put (instanceUUID, nodeUUID);
                    return invokeService (instanceUUID, //TODO: note this is a dangerous recursion... avoid this.
                                          methodName,
                                          dimeRequest,
                                          asynchronous);
                }
            }
            else {
                throw ex;
            }
        }
    }

    public boolean deactivateService (String serviceInstanceUUID) throws Exception
    {
        try {
            connect (false);
            return deactivateService (_commHelper, null, serviceInstanceUUID);
        }
        catch (CommException ex) {
            log ("Error communicating with the ACIKernel. Will try to reconnect");
            connect (true);
            return deactivateService (_commHelper, null, serviceInstanceUUID);
        }
    }

    public void deployService (URL acrFileURL) throws Exception
    {
        try {
            connect (false);
            deployService (_commHelper, acrFileURL);
        }
        catch (CommException ex) {
            log ("Error communicating with the ACIKernel. Will try to reconnect");
            connect (true);
            deployService (_commHelper, acrFileURL);
        }
    }

    public CommHelper connectToService (String serviceInstanceUUID) throws Exception
    {
        try {
            connect (false);
            return connectToService (_commHelper, serviceInstanceUUID);
        }
        catch (CommException ex) {
            log ("Error communicating with the ACIKernel. Will try to reconnect");
            connect (true);
            return connectToService (_commHelper, serviceInstanceUUID);
        }
    }

    public InetSocketAddress getNodeLocation (String nodeUUID) throws Exception
    {
        try {
            connect (false);
            return getNodeLocation (_commHelper, nodeUUID);
        }
        catch (CommException ex) {
            log ("Error communicating with the ACIKernel. Will try to reconnect");
            connect (true);
            return getNodeLocation (_commHelper, nodeUUID);
        }
    }

    public String lookupService (String instanceUUID, String oldNodeUUID) throws Exception
    {
        try {
            connect (false);
            return lookupService (_commHelper, instanceUUID, oldNodeUUID);
        }
        catch (CommException ex) {
            log ("Error communicating with the ACIKernel. Will try to reconnect");
            connect (true);
            return lookupService (_commHelper, instanceUUID, oldNodeUUID);
        }
    }

    public float getResourceInfo (int queryType, String serviceName, String methodSignature) throws Exception
    {
        try {
            connect (false);
            return getResourceInfo (_commHelper, queryType, serviceName, methodSignature);
        }
        catch (CommException ex) {
            log ("Error communicating with the ACIKernel. Will try to reconnect");
            connect (true);
            return getResourceInfo (_commHelper, queryType, serviceName, methodSignature);
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    // PROTECTED METHODS ///////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     *
     */
    protected void init (String kernelAddress, int kernelPort)
    {
        this.init(kernelAddress, kernelPort, false);
    } //init()

    /**
     *
     */
    protected void init (String kernelAddress, int kernelPort, boolean useMockets)
    {
        if (kernelAddress == null) {
            throw new NullPointerException ("kernelAddress");
        }

        _aciKernelAddress = kernelAddress;
        _aciKernelPort = kernelPort;

        _agServeUtils = new AgServeUtils();
    } //init()

    /**
     *
     */
    protected static CommHelper connect (String host, int port, boolean useMockets)
        throws IOException
    {
        if (useMockets) {
            return mocketConnect(host, port);
        }
        else {
            return socketConnect(host, port);
        }
    } //

    /**
     * Establishes a Socket connection to the ACIKernel listening on host:port
     */
    private static CommHelper socketConnect (String host, int port)
        throws IOException
    {
        Socket sock;
        if (BENCHMARK) {
            us.ihmc.aci.net.CustomSocketFactory sf = new us.ihmc.aci.net.CustomSocketFactory();
            sock = sf.createSocket(host, port);
        }
        else {
            sock = new Socket (host, port);
        }

        sock.setTcpNoDelay (true);

        if (_timeout > 0) {
            try {
                sock.setSoTimeout (_timeout);
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }

        CommHelper commHelper = new CommHelper (sock);
        return commHelper;
    } //connect()

    /**
     * Establishes a Mocket connection to the ACIKernel listening on host:port
     */
    private static CommHelper mocketConnect (String host, int port)
        throws IOException
    {
        StreamMocket mocket = new StreamMocket();
        mocket.connect(InetAddress.getByName(host), port, _timeout);

        CommHelper commHelper = new CommHelper();
        commHelper.init(mocket.getInputStream(), mocket.getOutputStream());

        return commHelper;
    } //connect()

    /**
     *
     */
    protected CommHelper connect (String nodeUUID, boolean forceReconnect)
        throws Exception
    {
        if (nodeUUID == null) {
            throw new NullPointerException (nodeUUID);
        }

        if ((_nodeUUIDToKernelAddressTable.get(nodeUUID) == null) || forceReconnect) {
            InetSocketAddress isa = getNodeLocation (nodeUUID);
            if (isa == null) {
                throw new Exception ("could not resolve location for node with UUID <" + nodeUUID +  ">");
            }
            else {
                log ("located node <" + nodeUUID + "> at address <"+ isa.getAddress() + ">");
            }

            CommHelper ce = null;
            if (_useMockets) {
                ce = mocketConnect(isa.getHostName(), isa.getPort());
            }
            else {
                ce = socketConnect(isa.getHostName(), isa.getPort());
            }

            if (! authenticate(ce)) {
                throw new IOException ("authentication failed");
            }
            _nodeUUIDToKernelAddressTable.put (nodeUUID, ce);
            return ce;
        }
        else {
            return (CommHelper) _nodeUUIDToKernelAddressTable.get (nodeUUID);
        }
    } //connect()

    /**
     *
     */
    protected  Dime invokeService (String nodeUUID, String instanceUUID, String methodName, Dime dimeRequest, boolean asynchronous)
        throws Exception
    {
        CommHelper ce;
        if (nodeUUID.equals(_kernelNodeUUID)) { //NOTE: currently kernelNodeUUID is not filled when using HTTP
            //the service is running in the "local" kernel.
            ce = _commHelper;
        }
        else {
            ce = connect (nodeUUID, false);
        }

        try {
            return invokeService (ce, (String) _svcLocations.get (instanceUUID),
                                  instanceUUID,
                                  methodName,
                                  dimeRequest,
                                  asynchronous);
        }
        catch (CommException ex) {
            // The connection may have timed out - before returning a failure,
            // try again by establishing a new connection
            log ("Error communicating with the ACIKernel (" + ex.getMessage() + "). Trying to reconnect");
            ce = connect (nodeUUID, true);
            return invokeService (ce, (String) _svcLocations.get (instanceUUID),
                                  instanceUUID,
                                  methodName,
                                  dimeRequest,
                                  asynchronous);
        }
    } //invokeService()

    /**
     *
     */
    protected boolean authenticate (CommHelper commHelper)
    {
        return true;
    } //authenticate()

    /**
     *
     */
    protected static void log (String msg)
    {
        if (DEBUG) {
            System.out.println ("[AgserveClient] " + msg);
        }
    } //log()

    // /////////////////////////////////////////////////////////////////////////
    // ABSTRACT METHODS ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    protected abstract String activateService (CommHelper commHelper, String nodeUUID, String serviceName, boolean onLocalNodeOnly) throws Exception;
    protected abstract boolean deactivateService (CommHelper commHelper, String nodeUUID, String serviceInstanceUUID) throws Exception;
    protected abstract void deployService (CommHelper commHelper, URL acrFileURL) throws Exception;
    protected abstract InetSocketAddress getNodeLocation (CommHelper commHelper, String nodeUUID) throws Exception;
    protected abstract String lookupService (CommHelper commHelper, String instanceUUID, String oldNodeUUID) throws Exception;
    protected abstract float getResourceInfo (CommHelper commHelper, int queryType, String serviceName, String methodSignature) throws Exception;
    protected abstract Dime invokeService (CommHelper commHelper, String nodeUUID, String instanceUUID, String methodName, Dime dimeRequest, boolean asynchronous) throws Exception;
    protected abstract CommHelper connectToService (CommHelper commHelper, String serviceInstanceUUID) throws Exception;

    // /////////////////////////////////////////////////////////////////////////
    private static final boolean DEBUG = true;
    private static final boolean BENCHMARK = false;

    // /////////////////////////////////////////////////////////////////////////
    private String _aciKernelAddress = "127.0.0.1";
    private int    _aciKernelPort = DEFAULT_ACI_KERNEL_PORT;
    protected static Hashtable _nodeUUIDToKernelAddressTable = new Hashtable();

    private boolean _useMockets = false;

    //maps instanceUUIDs to nodeUUIDs
    protected static Hashtable _svcLocations = new Hashtable();
    protected CommHelper _commHelper;

    protected AgServeUtils _agServeUtils;
    protected String _clientUUID;
    protected String _kernelNodeUUID = "";

    protected static int _timeout = 0;
    public static final int DEFAULT_ACI_KERNEL_PORT = 2005;
}
