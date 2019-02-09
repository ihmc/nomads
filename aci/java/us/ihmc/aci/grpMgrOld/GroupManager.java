package us.ihmc.aci.grpMgrOld;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URI;
import java.security.InvalidKeyException;
import java.security.KeyPair;
import java.security.NoSuchProviderException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.util.logging.Logger;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;
import java.util.Vector;

import us.ihmc.comm.CommHelper;
import us.ihmc.ds.Diff;
import us.ihmc.net.NetUtils;
import us.ihmc.util.Base64Transcoders;
import us.ihmc.util.ByteArray;
import us.ihmc.util.IDGenerator;
import us.ihmc.util.LogHelper;
import us.ihmc.util.crypto.CryptoUtils;
import us.ihmc.util.crypto.SecureOutputStream;


/**
 * The Group Manager is the component of the Agile Computing Infrastructure 
 * (ACI) that allows the discovery of neighboring nodes, their resources 
 * and services, without relying on a centralized registry. 
 * <p>
 * Nodes can be organized into different groups, as an approach to characterize 
 * the node ownership and to place constraints on resource sharing. 
 * <p>
 * The applications running on the top of the GM are notified about the 
 * appearance/disappearance of nodes, as well as changes in the groups 
 * membership (@see GroupManagerListener).
 *
 * @author Niranjan Suri
 * @author Matteo Rebeschini
 * @author Maggie Breedy
 * @version $Revision$
 * $Date$
 */
public class GroupManager extends Thread
{
    /**
     * Constructor used when creating a new instance of GroupManager without restoring
     * any settings from previous invocations.
     * <p>
     * Uses the default port number, assigns a new UUID, and starts with an empty
     * list of groups.
     */
    public GroupManager()
        throws GroupManagerException
    {
        firstTimeInit (DEFAULT_PORT, null);
    }

    /**
     * Constructor used when creating a new instance of GroupManager without restoring
     * any settings from previous invocations.
     * <p>
     * Uses the specified UUID, default port number, and starts with an empty
     * list of groups.
     */
    public GroupManager (String nodeUUID)
        throws GroupManagerException
    {
        _nodeUUID = nodeUUID;
        firstTimeInit (DEFAULT_PORT, null);
    }

    /**
     * Constructor used when creating a new instance of GroupManager without restoring
     * any settings from previous invocations.
     * <p>
     * Uses the specified port number, assigns a new UUID, and starts with an empty
     * list of groups.
     *
     * @param port
     * @throws GroupManagerException
     */
    public GroupManager (int port)
        throws GroupManagerException
    {
        firstTimeInit (port, null);
    }

    /**
     * Constructor used when creating a new instance of GroupManager without restoring
     * any settings from previous invocations.
     * <p>
     * Uses the specified port number, assigns a new UUID, and starts with an empty
     * list of groups.
     * <p>
     * The NewtorkAccess layer will use only the network interfaces specified in
     * <code>netIFsConf</code>
     *
     * @param port
     * @param netIFsConf Vector<NetworkInterfaceConf>
     * @throws GroupManagerException
     * @see NetworkInterfaceConf
     */
    public GroupManager (int port, Vector netIFsConf)
        throws GroupManagerException
    {
        firstTimeInit (port, netIFsConf);
    }

    /**
     * Constructor used when creating a new instance of GroupManager and resuming from
     * a previously stored state. 
     * <p>
     * This is the constructor that applications should use when there are 
     * previously saved settings.
     *
     * @param settings  a hashtable that contains the settings to be used by this instance
     */
    public GroupManager (Hashtable settings)
        throws GroupManagerException
    {
        init (settings);
    }
    
    /**
     * Constructor used when creating a new instance of GroupManager and resuming from
     * a previously stored state. 
     * <p>
     * This is the constructor that applications should use when there are 
     * previously saved settings.
     *
     * @param settings  a hashtable that contains the settings to be used by this instance
     * @param port
     * @param netIFsConf Vector<NetworkInterfaceConf>
     */
    public GroupManager (Hashtable settings, int port, Vector netIFsConf)
        throws GroupManagerException
    {
        init (settings, port, netIFsConf);
    }  

    /**
     * Returns the current state of the group manager. 
     * <p>
     * This state can be saved by the application and reused when restarting the 
     * Group Manager in the future.
     *
     * @return  a hashtable with the current settings
     * @throws  GroupManagerException
     */
    public Hashtable getSettingsSnapshot()
        throws GroupManagerException
    {
        Hashtable ht = new Hashtable();
        ht.put ("port", Integer.toString (_port));
        ht.put ("UUID", _nodeUUID);
        ht.put ("NodeName", _nodeName);
        ht.put ("StateSeqNo", Integer.toString (_infoStateSeqNo));
        ht.put ("GroupDataStateSeqNo", Integer.toString (_groupDataStateSeqNo));

        try {
            ht.put ("PublicKey",  Base64Transcoders.convertByteArrayToB64String (_keyPair.getPublic().getEncoded()));
            ht.put ("PrivateKey", Base64Transcoders.convertByteArrayToB64String (_keyPair.getPrivate().getEncoded()));
        }
        catch (IOException ioe) {
            throw new GroupManagerException ("failed to load public/private keys; nested exception - " + ioe);
        }

        // Add information about local public groups
        ht.put ("LocalPublicManagedGroupCount", Integer.toString (_publicManagedGroups.size()));
        int i = 0;
        Enumeration e = _publicManagedGroups.elements();
        while (e.hasMoreElements()) {
            LocalPublicManagedGroupInfo pgi = (LocalPublicManagedGroupInfo) e.nextElement();
            ht.put ("LocalPublicManagedGroup" + i, pgi.groupName);
            i++;
        }
        
        // Add information about local private groups
        ht.put ("LocalPrivateManagedGroupCount", Integer.toString (_privateManagedGroups.size()));
        i = 0;
        e = _privateManagedGroups.elements();
        while (e.hasMoreElements()) {
            LocalPrivateManagedGroupInfo rgi = (LocalPrivateManagedGroupInfo) e.nextElement();
            ht.put ("LocalPrivateManagedGroup" + i, rgi.groupName + ";" + rgi.password);
        }

        // Add information about public peer groups
        ht.put ("LocalPublicPeerGroupCount", Integer.toString (_publicPeerGroups.size()));
        i = 0;
        e = _publicPeerGroups.elements();
        while (e.hasMoreElements()) {
            LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) e.nextElement();
            ht.put ("LocalPublicPeerGroup" + i, ppgi.groupName);
        }

        // Add information about private peer groups
        ht.put ("LocalPrivatePeerGroupCount", Integer.toString (_privatePeerGroups.size()));
        i = 0;
        e = _privatePeerGroups.elements();
        while (e.hasMoreElements()) {
            LocalPrivatePeerGroupInfo rpgi = (LocalPrivatePeerGroupInfo) e.nextElement();
            ht.put ("LocalPrivatePeerGroup" + i, rpgi.groupName + ";" + rpgi.password);
        }
        return ht;
    }

    /**
     * Initializes the Group Manager resuming from a previously stored state.
     *
     * @param settings  a hashtable that contains the settings to be used by this instance
     * @throws GroupManagerException
     */
    public void init (Hashtable settings)
        throws GroupManagerException
    {
        try {
            CryptoUtils.setProvider ("BC");
        }
        catch (NoSuchProviderException e) {
            throw new GroupManagerException ("Could not find BouncyCastle provider for JCE; nested exception - " + e);
        }

        // Initialize the logger
        _logger = LogHelper.getLogger ("us.ihmc.aci.grpMgr.GroupManager");

        String value;

        // Get the port setting
        value = (String) settings.get ("port");
        if (value == null) {
            throw new GroupManagerException ("could not find port property in settings");
        }
        _port = Integer.parseInt (value);

        // Get the UUID
        value = (String) settings.get ("UUID");
        if (value == null) {
            throw new GroupManagerException ("could not find nodeUUID property in settings");
        }
        _nodeUUID = value;

        // Get the NodeName
        value = (String) settings.get ("NodeName");
        if (value == null) {
            throw new GroupManagerException ("could not find NodeName property in settings");
        }
        _nodeName = value;

        // Get the last state sequence number
        value = (String) settings.get ("StateSeqNo");
        if (value == null) {
            throw new GroupManagerException ("could not find state sequence number property in settings");
        }
        _infoStateSeqNo = Integer.parseInt (value) + 1;

        // Get the last group data sequence number
        value = (String ) settings.get ("GroupDataStateSeqNo");
        if (value == null) {
            throw new GroupManagerException ("could not find group data state sequence number property in settings");
        }
        _groupDataStateSeqNo = Integer.parseInt (value) + 1;

        // Get the Public Key Pair representing this Group Manager
        try {
            value = (String) settings.get ("PublicKey");
            PublicKey pubKey = CryptoUtils.createPublicKeyFromDEREncodedX509Data (
                    Base64Transcoders.convertB64StringToByteArray(value));
            value = (String) settings.get ("PrivateKey");
            PrivateKey privKey = CryptoUtils.createPrivateKeyFromDEREncodedPKCS8Data (
                    Base64Transcoders.convertB64StringToByteArray(value));
        }
        catch (Exception e) {
            throw new GroupManagerException ("failed to load or recreate public/private keys; nested exception - " + e);
        }

        // Initialize the Network Access Layer
        _nal = new NetworkAccessLayer();
        try {
            _nal.init (_nodeUUID, _port, null, this);
        }
        catch (NetworkException ne) {
            throw new GroupManagerException ("Could not initialize NetworkAccessLayer; nested exception - " + ne);
        }

        // Get the Public Key Pair representing this Group Manager
        try {
            value = (String) settings.get ("PublicKey");
            PublicKey pubKey = CryptoUtils.createPublicKeyFromDEREncodedX509Data (
                    Base64Transcoders.convertB64StringToByteArray(value));
            value = (String) settings.get ("PrivateKey");
            PrivateKey privKey = CryptoUtils.createPrivateKeyFromDEREncodedPKCS8Data (
                    Base64Transcoders.convertB64StringToByteArray(value));
            _keyPair = new KeyPair (pubKey, privKey);
        }
        catch (Exception e) {
            throw new GroupManagerException ("failed to load or recreate public/private keys; nested exception - " + e);
        }

        // Initialize the TCP Server (used for manager groups)
        _tcpServer = new TCPServer();
        try {
            _tcpServer.init (_nodeUUID, _port, this, _keyPair);
        }
        catch (NetworkException ne) {
           throw new GroupManagerException ("Could not initialize TCPServer; nested exception - " + ne);
        }

        _peers = new Hashtable();
        _deadPeers = new Hashtable();
        _groups = new Hashtable();

        // Get the local public managed groups
        value = (String) settings.get ("LocalPublicManagedGroupCount");
        if (value != null) {
            int count = Integer.parseInt (value);
            _publicManagedGroups = new Hashtable();
            for (int i = 0; i < count; i++) {
                value = (String) settings.get ("LocalPublicManagedGroup" + i);
                if (value == null) {
                    throw new GroupManagerException ("could not find name for local public group " + i);
                }
                createPublicManagedGroup (value);
            }
        }

        // Get the local private managed groups
        value = (String) settings.get ("LocalPrivateManagedGroupCount");
        if (value != null) {
            int count = Integer.parseInt (value);
            _privateManagedGroups = new Hashtable();
	        for (int i = 0; i < count; i++) {
	            value = (String) settings.get ("LocalPrivateManagedGroup" + i);
	            if (value == null) {
	                throw new GroupManagerException ("could not find information about local private group " + i);
	            }
	            StringTokenizer st = new StringTokenizer (value, ";");
	            String groupName = null;
	            String password = null;
	            try {
	                groupName = st.nextToken();
	                password = st.nextToken();
	            }
	            catch (NoSuchElementException e) {};
	            if ((groupName == null) || (password == null)) {
	                throw new GroupManagerException ("could not find all information for local private group " + i);
	            }
	            createPrivateManagedGroup (groupName, password);
	        }
	    }
	
	    // Get the local public peer groups
	    value = (String) settings.get ("LocalPublicPeerGroupCount");
	    if (value != null) {
	        int count = Integer.parseInt (value);
	        _publicPeerGroups = new Hashtable();
	        for (int i = 0; i < count; i++) {
	            value = (String) settings.get ("LocalPublicPeerGroup" + i);
	            if (value == null) {
	                throw new GroupManagerException ("could not find name for public peer group " + i);
	            }
	            createPublicPeerGroup (value, null, 0, 0);
	        }
	    }
	
	    // Get the local private peer groups
	    value = (String) settings.get ("LocalPrivatePeerGroupCount");
	    if (value != null) {
	        int count = Integer.parseInt (value);
	        _privatePeerGroups = new Hashtable();
	        for (int i = 0; i < count; i++) {
	            value = (String) settings.get ("LocalPrivatePeerGroup" + i);
	            if (value == null) {
	                throw new GroupManagerException ("could not find information about private peer group " + i);
	            }
	            StringTokenizer st = new StringTokenizer (value, ";");
	            String groupName = null;
	            String password = null;
	            try {
	                groupName = st.nextToken();
	                password = st.nextToken();
	            }
	            catch (NoSuchElementException e) {};
	            if ((groupName == null) || (password == null)) {
	                throw new GroupManagerException ("could not find all the information for private peer group " + i);
	            }
	            createPrivatePeerGroup (groupName, password, null, 0, 0);
	        }
	    }
	
	    compileGroupList();
	}
    
    /**
     * Enables/Disables the node advertisement. If the node advertisement is disabled
     * PING/INFO packet are not sent in a regular basis, and the GroupManager is used
     * only for peer searches.
     * 
     * @param advertiseNode
     */
    public void setNodeAdvertisement (boolean advertiseNode)
    {
    	_advertiseNode = advertiseNode;
    }

    /**
     * Initializes the Group Manager resuming from a previously stored state.
     *
     * @param settings  a hashtable that contains the settings to be used by this instance
     * @param port
     * @param netIFsConf Vector<NetworkInterfaceConf>
     * @throws GroupManagerException
     */
    public void init (Hashtable settings, int port, Vector netIFsConf)
        throws GroupManagerException
    {
        try {
            CryptoUtils.setProvider ("BC");
        }
        catch (NoSuchProviderException e) {
            throw new GroupManagerException ("Could not find BouncyCastle provider for JCE; nested exception - " + e);
        }

        // Initialize the logger
        _logger = LogHelper.getLogger ("us.ihmc.aci.grpMgr.GroupManager");
        _port = port;
        
        String value;
        // Get the UUID
        value = (String) settings.get ("UUID");
        if (value == null) {
            throw new GroupManagerException ("could not find nodeUUID property in settings");
        }
        _nodeUUID = value;

        // Get the NodeName
        value = (String) settings.get ("NodeName");
        if (value == null) {
            throw new GroupManagerException ("could not find NodeName property in settings");
        }
        _nodeName = value;

        // Get the last state sequence number
        value = (String) settings.get ("StateSeqNo");
        if (value == null) {
            throw new GroupManagerException ("could not find state sequence number property in settings");
        }
        _infoStateSeqNo = Integer.parseInt (value) + 1;

        // Get the last group data sequence number
        value = (String ) settings.get ("GroupDataStateSeqNo");
        if (value == null) {
            throw new GroupManagerException ("could not find group data state sequence number property in settings");
        }
        _groupDataStateSeqNo = Integer.parseInt (value) + 1;

        // Initialize the Network Access Layer
        _nal = new NetworkAccessLayer();
        try {
            _nal.init (_nodeUUID, _port, netIFsConf, this);
        }
        catch (NetworkException ne) {
            throw new GroupManagerException ("Could not initialize NetworkAccessLayer; nested exception - " + ne);
        }

        // Get the Public Key Pair representing this Group Manager
        try {
            value = (String) settings.get ("PublicKey");
            PublicKey pubKey = CryptoUtils.createPublicKeyFromDEREncodedX509Data (
                    Base64Transcoders.convertB64StringToByteArray(value));
            value = (String) settings.get ("PrivateKey");
            PrivateKey privKey = CryptoUtils.createPrivateKeyFromDEREncodedPKCS8Data (
                    Base64Transcoders.convertB64StringToByteArray(value));
            _keyPair = new KeyPair (pubKey, privKey);
        }
        catch (Exception e) {
            throw new GroupManagerException ("failed to load or recreate public/private keys; nested exception - " + e);
        }

        // Initialize the TCP Server (used for manager groups)
        _tcpServer = new TCPServer();
        try {
            _tcpServer.init (_nodeUUID, _port, this, _keyPair);
        }
        catch (NetworkException ne) {
           throw new GroupManagerException ("Could not initialize TCPServer; nested exception - " + ne);
        }

        _peers = new Hashtable();
        _deadPeers = new Hashtable();
        _groups = new Hashtable();

        // Get the local public managed groups
        value = (String) settings.get ("LocalPublicManagedGroupCount");
        if (value != null) {
            int count = Integer.parseInt (value);
            _publicManagedGroups = new Hashtable();
            for (int i = 0; i < count; i++) {
                value = (String) settings.get ("LocalPublicManagedGroup" + i);
                if (value == null) {
                    throw new GroupManagerException ("could not find name for local public group " + i);
                }
                createPublicManagedGroup (value);
            }
        }

        // Get the local private managed groups
        value = (String) settings.get ("LocalPrivateManagedGroupCount");
        if (value != null) {
            int count = Integer.parseInt (value);
            _privateManagedGroups = new Hashtable();
	        for (int i = 0; i < count; i++) {
	            value = (String) settings.get ("LocalPrivateManagedGroup" + i);
	            if (value == null) {
	                throw new GroupManagerException ("could not find information about local private group " + i);
	            }
	            StringTokenizer st = new StringTokenizer (value, ";");
	            String groupName = null;
	            String password = null;
	            try {
	                groupName = st.nextToken();
	                password = st.nextToken();
	            }
	            catch (NoSuchElementException e) {};
	            if ((groupName == null) || (password == null)) {
	                throw new GroupManagerException ("could not find all information for local private group " + i);
	            }
	            createPrivateManagedGroup (groupName, password);
	        }
	    }
	
	    // Get the local public peer groups
	    value = (String) settings.get ("LocalPublicPeerGroupCount");
	    if (value != null) {
	        int count = Integer.parseInt (value);
	        _publicPeerGroups = new Hashtable();
	        for (int i = 0; i < count; i++) {
	            value = (String) settings.get ("LocalPublicPeerGroup" + i);
	            if (value == null) {
	                throw new GroupManagerException ("could not find name for public peer group " + i);
	            }
	            createPublicPeerGroup (value, null, 0, 0);
	        }
	    }
	
	    // Get the local private peer groups
	    value = (String) settings.get ("LocalPrivatePeerGroupCount");
	    if (value != null) {
	        int count = Integer.parseInt (value);
	        _privatePeerGroups = new Hashtable();
	        for (int i = 0; i < count; i++) {
	            value = (String) settings.get ("LocalPrivatePeerGroup" + i);
	            if (value == null) {
	                throw new GroupManagerException ("could not find information about private peer group " + i);
	            }
	            StringTokenizer st = new StringTokenizer (value, ";");
	            String groupName = null;
	            String password = null;
	            try {
	                groupName = st.nextToken();
	                password = st.nextToken();
	            }
	            catch (NoSuchElementException e) {};
	            if ((groupName == null) || (password == null)) {
	                throw new GroupManagerException ("could not find all the information for private peer group " + i);
	            }
	            createPrivatePeerGroup (groupName, password, null, 0, 0);
	        }
	    }
	
	    compileGroupList();
	}

    
	/**
	 * The run() method for the thread. 
	 */
	public void run()
	{
	    _running = true;
	
	    // Start NetworkAccessLayer
	    _nal.start();
	    
	    // Start the TCP server
	    _tcpServer.start();
	
	    // Send the first info message
	    updateInfoMessage();
	    sendInfoMessage();
	
	    // Begin the main loop
	    while (!_terminate) {
	        try {            
	            Thread.sleep (1000);
	        }
	        catch (InterruptedException ie) {
	            _terminatingException = ie;
	            ie.printStackTrace();
	        }
	        
	        if (_advertiseNode) {
		        synchronized (this) 
		        {
		            // Send a ping message every 2 seconds
		            long currTime = System.currentTimeMillis();
		            if ((currTime - _lastInfoBCastTime) > _infoBCastInterval) {
		                sendInfoMessage();     // Will update _lastBCastTime
		            }
		            else if ((currTime - _lastPingTime) > _pingInterval) {
		                sendPingMessage();     // Will update _lastPingTime
		            }
		            timeoutNodes();
		            timeoutPersistentPeerSearches();
		        }
	        }
	    }
	
	    _nal.terminate();
	    _running = false;
	    _terminated = true;
	}

	/**
	 * Set the listener for Group Manager events.
	 *
	 * @param gml  the listener
	 */
	public void setListener (GroupManagerListener gml)
	{
	    _listener = gml;
	}
	
	/**
	 * Returns the running state of this GroupManager thread.
	 *
	 * @return     <code>true</code> if the instance is running, false otherwise
	 */
	public boolean isRunning()
	{
	    return _running;
	}
	
	/**
	 * Request the GroupManager thread to terminate. 
	 * <p>
	 * Caller should wait until hasTerminated() returns <code>true</code>.
	 */
	public void terminate()
	{
	    _terminate = true;
	}
	
	/**
	 * Check whether the GroupManager thread if it has terminated
	 *
	 * @return     <code>true</code> if the instance has terminated, false otherwise
	 */
	public boolean hasTerminated()
	{
	    return _terminated;
	}
	
	/**
	 * Returns the exception (if any) that caused the GroupManager 
	 * thread to terminate
	 *
	 * @return     the exception or null
	 */
	public Exception getTerminatingException()
	{
	    return _terminatingException;
	}
		
	/**
	 * Create a Public Managed Group
	 *
	 * @param groupName name of the group: it cannot contain spaces and 
	 *                  each group has to have a unique name
	 * @throws GroupManagerException
	 */
	public synchronized void createPublicManagedGroup (String groupName)
	    throws GroupManagerException
	{
	    _logger.info ("creating public managed group: " + groupName);
	
	    if (!validateGroupName (groupName)) {
	        throw new GroupManagerException ("invalid group name <" + groupName + ">");
	    }
	
	    LocalPublicManagedGroupInfo lpmgi = new LocalPublicManagedGroupInfo();
	    lpmgi.groupName = groupName;
	    _publicManagedGroups.put (groupName, lpmgi);
	    updateInfoMessage();
	    if (_running) {
	        sendInfoMessage();
	    }
	    compileGroupList();
	}

	/**
	 * Create a Private Managed Group
	 *
	 * @param groupName      name of the group: it cannot contain spaces and 
	 *                       each group has to have a unique name
	 * @param password       password for the group
	 * @throws GroupManagerException
	 */
	public synchronized void createPrivateManagedGroup (String groupName, String password)
	    throws GroupManagerException
	{
	    _logger.info ("creating private managed group: " + groupName);
	
	    if (!validateGroupName (groupName)) {
	        throw new GroupManagerException ("invalid group name <" + groupName + ">");
	    }
	
	    if (!validatePassword (password)) {
	        throw new GroupManagerException ("invalid password <" + password + ">");
	    }
	
	    try {
	        LocalPrivateManagedGroupInfo lpmgi = new LocalPrivateManagedGroupInfo();
	        lpmgi.groupName = groupName;
	        lpmgi.password = password;
	        lpmgi.key = CryptoUtils.generateSecretKey (password);
	        lpmgi.nonce = IDGenerator.generateID();
	        lpmgi.encryptedNonce = Base64Transcoders.convertByteArrayToB64String (
	                CryptoUtils.encryptStringUsingSecretKey (lpmgi.key, lpmgi.nonce));
	        _privateManagedGroups.put (groupName, lpmgi);
	        updateInfoMessage();
	        if (_running) {
	            sendInfoMessage();
	        }
	        compileGroupList();
	    }
	    catch (InvalidKeyException e) {
	        throw new GroupManagerException ("failed to create group; nested exception = " + e);
	    }
	    catch (IOException e) {
	        throw new GroupManagerException ("failed to create group; nested exception = " + e);
	    }
	}

	/**
	 * Create a Public Peer Group 
	 *
	 * @param groupName      name of the group - it cannot contain spaces and 
	 *                       each group has to have a unique name
	 * @param data           application defined data byte array attached to the group - can be null
	 * @param hopCount       hop count - limits the number of hops through which 
	 *                       the packet is propagated (1-255)
	 * @param floodProb      flood probability - controls the willingness of a 
	 *                       node to retransmit the packet (0-100)
	 * @throws GroupManagerException
	 */
	public synchronized void createPublicPeerGroup (String groupName, byte[] data, int hopCount, int floodProb)
	    throws GroupManagerException
	{
	    _logger.info ("creating public peer group: " + groupName);
	
	    if (!validateGroupName (groupName)) {
	        throw new GroupManagerException ("invalid group name <" + groupName + ">");
	    }
	
	    LocalPublicPeerGroupInfo ppgi = new LocalPublicPeerGroupInfo();
	    ppgi.groupName = groupName;
	    ppgi.hopCount = hopCount;
	    ppgi.floodProbability = floodProb;
	    if ((data != null) && (data.length > 0)) {
	    	ppgi.data = (byte[]) data.clone();
        }
        _publicPeerGroups.put (groupName, ppgi);
        updateInfoMessage();
        if (_running) {
            sendInfoMessage();
        }
        compileGroupList();

        // Check peer groups at other group managers to see if any of them should be added as members
        for (Enumeration e = _peers.elements(); e.hasMoreElements();) {
            PeerInfo pi = (PeerInfo) e.nextElement();
            for (Enumeration en = pi.groups.elements(); en.hasMoreElements();) {
                RemoteGroupInfo rgi = (RemoteGroupInfo) en.nextElement();
                if (rgi.groupName.equals (groupName)) {
                    if (rgi instanceof RemotePublicPeerGroupInfo) {
                        addRemotePublicPeerGroup (ppgi, (RemotePublicPeerGroupInfo) rgi);
                    }
                    else if (rgi instanceof RemotePrivatePeerGroupInfo) {
                        if (_listener != null) {
                            _listener.conflictWithPrivatePeerGroup (groupName, pi.nodeUUID);
                        }
                    }
                }
            }
        }
    }

    /**
     * Create a Public Peer Group
     * <p>
     * The default flood probability will be used (100).
     *
     * @param groupName      name of the group - it cannot contain spaces and
     *                       each group has to have a unique name
     * @param data           application defined data byte array attached to the group - can be null
     * @param hopCount       hop count - limits the number of hops through which
     *                       the packet is propagated (1-255)
     */
    public synchronized void createPublicPeerGroup (String groupName, byte[] data, int hopCount)
        throws GroupManagerException
    {
        createPublicPeerGroup (groupName, data, hopCount, _pingFloodProb);
    }

    /**
     * Create a Public Peer Group
     * <p>
     * The deafault hopCount (1) and floodProbability (100) will be used.
     *
     * @param groupName      name of the group - it cannot contain spaces and
     *                       each group has to have a unique name
     * @param data           application defined data byte array attached to the group - can be null
     */
    public synchronized void createPublicPeerGroup (String groupName, byte[] data)
        throws GroupManagerException
    {
        createPublicPeerGroup (groupName, data, _pingHopCount, _pingFloodProb);
    }

    /**
     * Create a Private Peer Group 
     *
     * @param groupName      name of the group - it cannot contain spaces and
     *                       each group has to have a unique name
     * @param password       password for the private peer group
     * @param data           application defined data byte array attached to the group - can be null
     * @param hopCount       hop count - limits the number of hops through which
     *                       the packet is propagated (1-255)
     * @param floodProb      flood probability - controls the willingness of a
     *                       node to retransmit the packet (0-100)
     * @throws GroupManagerException
     */
    public synchronized void createPrivatePeerGroup (String groupName, String password, byte[] data,
                                                     int hopCount, int floodProb)
        throws GroupManagerException
    {
        _logger.info ("creating private peer group: " + groupName);

        if (!validateGroupName (groupName)) {
            throw new GroupManagerException ("invalid group name <" + groupName + ">");
        }

        if (!validatePassword (password)) {
            throw new GroupManagerException ("invalid password <" + password + ">");
        }

        LocalPrivatePeerGroupInfo lppgi = new LocalPrivatePeerGroupInfo();
        try {
            lppgi.groupName = groupName;
            lppgi.password = password;
            lppgi.hopCount = hopCount;
            lppgi.floodProbability = floodProb;
            lppgi.key = CryptoUtils.generateSecretKey (password);
            lppgi.nonce = IDGenerator.generateID();
            lppgi.encryptedGroupName = Base64Transcoders.convertByteArrayToB64String (
                    CryptoUtils.encryptStringUsingSecretKey (lppgi.key, lppgi.groupName));
            lppgi.encryptedNonce = Base64Transcoders.convertByteArrayToB64String (
                    CryptoUtils.encryptStringUsingSecretKey (lppgi.key, lppgi.nonce));

            if ((data != null) && (data.length > 0)) {
                lppgi.data = (byte[]) data.clone();
            }
            // Get the Encrypted data from the data
            if (lppgi.data != null) {
                lppgi.encryptedData = CryptoUtils.encryptUsingSecretKey (lppgi.key, lppgi.data);
            }
            _privatePeerGroups.put (groupName, lppgi);
            updateInfoMessage();
            if (_running) {
                sendInfoMessage();
            }
            compileGroupList();
        }
        catch (InvalidKeyException e) {
            throw new GroupManagerException ("failed to create group; nested exception = " + e);
        }
        catch (IOException e) {
            throw new GroupManagerException ("failed to create group; nested exception = " + e);
        }
        // Check peer groups at other group managers to see if any of them should be added as members
        for (Enumeration e = _peers.elements(); e.hasMoreElements();) {
            PeerInfo pi = (PeerInfo) e.nextElement();
            for (Enumeration en = pi.groups.elements(); en.hasMoreElements();) {
                RemoteGroupInfo rgi = (RemoteGroupInfo) en.nextElement();
                if (rgi.groupName.equals (groupName)) {
                    if (rgi instanceof RemotePublicPeerGroupInfo) {
                        if (_listener != null) {
                            _listener.conflictWithPrivatePeerGroup (groupName, pi.nodeUUID);
                        }
                    }
                    else if (rgi instanceof RemotePrivatePeerGroupInfo) {
                        addRemotePrivatePeerGroup (lppgi, (RemotePrivatePeerGroupInfo) rgi);
                    }
                }
            }
        }
    }

    /**
     * Create a Private Peer Group
     * <p>
     * The default flood probability will be used (100)
     *
     * @param groupName      name of the group - it cannot contain spaces and
     *                       each group has to have a unique name
     * @param password       password for the private peer group
     * @param data           application defined data byte array attached to the group - can be null
     * @param hopCount       hop count - limits the number of hops through which
     *                       packet is propagated (1-255)
     * @throws GroupManagerException
     */
    public synchronized void createPrivatePeerGroup (String groupName, String password, byte[] data, int hopCount)
        throws GroupManagerException
    {
        createPrivatePeerGroup (groupName, password, data, hopCount, _pingFloodProb);
    }

    /**
     * Create a Private Peer Group 
     * <p>
     * The default hop count (1) and flood probability will be used (100).
     *
     * @param groupName      name of the group - it cannot contain spaces and
     *                       each group has to have a unique name
     * @param password       password for the private peer group
     * @param data           application defined data byte array attached to the group - can be null
     * @throws GroupManagerException
     */
    public synchronized void createPrivatePeerGroup (String groupName, String password, byte[] data)
        throws GroupManagerException
    {
        createPrivatePeerGroup (groupName, password, data, _pingHopCount, _pingFloodProb);
    }

    /**
     * Change the data associated with a peer group
     *
     * @param groupName     name of the group for which the data should be updated - it cannot 
     *                      contain spaces and each group has to have a unique name
     * @param data          application defined data byte array
     * @param hopCount      hop count - limits the number of hops through which
     *                      the packet is propagated (1-255)
     * @param floodProb     flood probability - controls the willingness of a
     *                      node to retransmit the packet (0-100)
     * @throws GroupManagerException
     */
    public synchronized void updatePeerGroupData (String groupName, byte[] data, int hopCount, int floodProb)
        throws GroupManagerException
    {
        GroupInfo gi = null;
        if ((gi = (GroupInfo) _publicPeerGroups.get (groupName)) != null) {
            LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) gi;
            if (data != null && data.length > 0) {
                ppgi.data = (byte[]) data.clone();
            }
            _groupDataStateSeqNo = ((_groupDataStateSeqNo+1) < MAX_SEQUENCE_NUM) ? (_groupDataStateSeqNo+1) : 0;
            sendPeerGroupDataMessage (groupName, ppgi.data.length, ppgi.data, 
                                      hopCount, floodProb, _groupDataStateSeqNo);
        }
        else if ((gi = (GroupInfo) _privatePeerGroups.get (groupName)) != null) {
            LocalPrivatePeerGroupInfo lppgi = (LocalPrivatePeerGroupInfo) gi;
            if (data != null && data.length > 0) {
                try {
                    lppgi.encryptedData = CryptoUtils.encryptUsingSecretKey (lppgi.key, data);
                }
                catch (InvalidKeyException e) {
                    throw new GroupManagerException ("could not encrypt data - nested exception = " + e);
                }
                if (lppgi.encryptedData == null) {
                    // An error occurred in encrypting the data
                    throw new GroupManagerException ("error in encrypting the data");
                }
                lppgi.data = (lppgi.data != null) ? (byte[]) data.clone() : null;
            }
            _groupDataStateSeqNo = ((_groupDataStateSeqNo+1) < MAX_SEQUENCE_NUM) ? (_groupDataStateSeqNo+1) : 0;
            sendPeerGroupDataMessage (groupName, lppgi.data.length, lppgi.encryptedData, 
                                      hopCount, floodProb, _groupDataStateSeqNo);
        }
        else {
            throw new GroupManagerException ("the specified group <" + groupName + "> does not exist");
        }
    }

    /**
     * Change the data associated with a peer group
     *
     * @param groupName      name of the group for which the data should be updated - it 
     *                       cannot contain spaces and each group has to have a unique name
     * @param data           data associated with the group
     * @param hopCount       hop count  - limits the number of hops through which
     *                       the packet is propagated (1-255)
     *
     * @throws GroupManagerException
     */
    public synchronized void updatePeerGroupData (String groupName, byte[] data, int hopCount)
        throws GroupManagerException
    {
        updatePeerGroupData (groupName, data, hopCount, _pingFloodProb);
    }

    /**
     * Change the data associated with a peer group
     *
     * @param groupName      name of the group for which the data should be updated - it
     *                       cannot contain spaces and each group has to have a unique name
     * @param data           data associated with the group
     * @throws GroupManagerException
     */
    public synchronized void updatePeerGroupData (String groupName, byte[] data)
        throws GroupManagerException
    {
        updatePeerGroupData (groupName, data, _pingHopCount, _pingFloodProb);
    }

    /**
     * Join Public Managed Group
     *
     * @param groupName     name of the group
     * @param creatorUUID   nodeUUID of the node that created the group
     * @throws GroupManagerException
     */
    public void joinPublicManagedGroup (String groupName, String creatorUUID)
        throws GroupManagerException
    {
        joinPublicManagedGroup (groupName, creatorUUID, JoinInfo.JOIN_ONE_TIME_ONLY, null, 0);
    }

    /**
     * Join Public Managed Group
     *
     * @param groupName     name of the group
     * @param creatorUUID   nodeUUID of the node that created the group
     * @param joinMode      join mode (@see GroupInfo)
     * @throws GroupManagerException
     */
    public void joinPublicManagedGroup (String groupName, String creatorUUID, int joinMode)
        throws GroupManagerException
    {
        joinPublicManagedGroup (groupName, creatorUUID, joinMode, null, 0);
    }

    /**
     * Join a Public Managed Group
     *
     * @param groupName     name of the group
     * @param creatorUUID   nodeUUID of the node that created the group
     * @param joinMode      join mode (@see GroupInfo)
     * @param joinData      application defined data byte array
     * @param joinDataLen   length of the application defined data byte array 
     * @throws GroupManagerException
     */
    public synchronized void joinPublicManagedGroup (String groupName, String creatorUUID, int joinMode,
                                                     byte[] joinData, int joinDataLen)
        throws GroupManagerException
    {
        _logger.info ("joining public managed group: " + groupName + " at peer " + creatorUUID);

        if (_joinedPublicManagedGroups.get (groupName) != null) {
            throw new GroupManagerException ("already joined public group <" + groupName + ">");
        }
        PublicManagedGroupJoinInfo ji = new PublicManagedGroupJoinInfo();
        ji.groupName = groupName;
        ji.creatorUUID = creatorUUID;
        ji.joinData = joinData;
        ji.joinDataLen = joinDataLen;
        ji.joinMode = joinMode;
        joinPublicManagedGroupAtPeer (ji);
        _joinedPublicManagedGroups.put (groupName, ji);
    }

    /**
     * Join a Private Managed Group
     *
     * @param groupName     name of the group
     * @param creatorUUID   nodeUUID of the node that created the group
     * @param password      password for the private managed group
     * @throws GroupManagerException
     */
    public void joinPrivateManagedGroup (String groupName, String creatorUUID, String password)
        throws GroupManagerException
    {
        joinPrivateManagedGroup (groupName, creatorUUID, password, JoinInfo.JOIN_ONE_TIME_ONLY, null, 0);
    }

    /**
     * Join a Private Managed Group
     *
     * @param groupName     name of the group
     * @param creatorUUID   nodeUUID of the node that created the group
     * @param password      password for the private managed group 
     * @param joinMode      join mode (@see GroupInfo)
     * @throws GroupManagerException
     */
    public void joinPrivateManagedGroup (String groupName, String creatorUUID, String password, int joinMode)
        throws GroupManagerException
    {
        joinPrivateManagedGroup (groupName, creatorUUID, password, joinMode, null, 0);
    }

    /**
     * Join a private group with the specified group name.
     *
     * @param groupName     name of the group
     * @param creatorUUID   nodeUUID of the node that created the group
     * @param password      password for the private managed group
     * @param joinMode      join mode (see GroupInfo)
     * @param joinData      application defined data byte array - can be null
     * @param joinDataLen   length of the application defined data byte array
     * @throws GroupManagerException
     */
    public synchronized void joinPrivateManagedGroup (String groupName, String creatorUUID, String password, int joinMode,
                                                      byte[] joinData, int joinDataLen)
        throws GroupManagerException
    {
        _logger.info ("joining private managed group: " + groupName + " at peer " + creatorUUID);

        if (_joinedPrivateManagedGroups.get (groupName) != null) {
            throw new GroupManagerException ("already joined private group <" + groupName + ">");
        }
        PrivateManagedGroupJoinInfo pmgji = new PrivateManagedGroupJoinInfo();
        pmgji.groupName = groupName;
        pmgji.creatorUUID = creatorUUID;
        pmgji.joinData = joinData;
        pmgji.joinDataLen = joinDataLen;
        pmgji.joinMode = joinMode;
        pmgji.password = password;
        joinPrivateManagedGroupAtPeer (pmgji);
        _joinedPrivateManagedGroups.put (groupName, pmgji);
    }

    /**
     * Leave the group identified by the group name
     *
     * @param groupName     name of the group
     * @throws GroupManagerException
     */
    public synchronized void leaveGroup (String groupName)
        throws GroupManagerException
    {
        JoinInfo ji = (JoinInfo) _joinedPublicManagedGroups.remove (groupName);
        if (ji == null) {
            ji = (JoinInfo) _joinedPrivateManagedGroups.remove (groupName);
            if (ji == null) {
                throw new GroupManagerException ("have not joined group <" + groupName + ">");
            }
        }
        leaveGroupAtPeer (ji);
    }

    /**
     * Start a peer search within the context of the specified 
     * group using the specified parameter
     * <p>
     * If the group name identifies a private group, the search 
     * request is encrypted using the password for the group)
     *
     * @param groupName         name of the group
     * @param hopCount          specifies the number of hops (retransmissions) that must be performed for
     *                          the search request (from 1 to 255)
     * @param floodProbability  probability (from 0 to 100) specifies the probability with which a node
     *                          that receives the search request will retransmit the request
     * @param param             the application defined parameter
     * @param peerSearchTTL     the time to live of the peer search
     * @return                  returns an UUID that represents the search request
     */
    public synchronized String startPeerSearch (String groupName, byte[] param, int hopCount,
                                                int floodProb, int peerSearchTTL)
        throws GroupManagerException
    {
        byte[] peerSearchMessage = new byte [MessageCodec.MAX_MESSAGE_SIZE];
        String searchUUID = IDGenerator.generateID();
        int messageType;
        boolean bPrivate;

        if (param == null) {
            throw new GroupManagerException ("Failed to search group <" + groupName +
                                             ">: cannot initiate a peer search with a null param");
        }

        /** If groupName is a Public Peer Group create a PEER_SEARCH_MESSAGE, otherwise if it is a
         *  Private Peer Group create a PEER_SEARCH_PPG_MESSAGE. These messages are broadcasted. Each node
         *  receiving them will rebroadcast them (if certain conditions are met) and the information contained
         *  in them (i.e. the search param) will be processed only if the node receiving the message is a member
         *  of groupName
         */
        GroupInfo gi = null;
        int messageLen = 0;
        if ((gi = (GroupInfo) _publicPeerGroups.get (groupName)) != null) {
            messageType = MessageCodec.PEER_SEARCH_MESSAGE;
            bPrivate = false;
            _logger.fine ("searching public peer group: " + groupName + ", searchUUID: " +
                          searchUUID + ", search string is: " + new String (param) + " (TTL: " +
                          peerSearchTTL + ")");

            messageLen = MessageCodec.createPeerSearchMessage (peerSearchMessage, _nodeUUID,  getActiveNICsInfo(), 
                                                               peerSearchTTL, searchUUID, _keyPair.getPublic(), 
                                                               groupName, param);
        }
        else if ((gi = (GroupInfo) _privatePeerGroups.get (groupName)) != null) {
            messageType = MessageCodec.PEER_SEARCH_PPG_MESSAGE;
            bPrivate = true;
            _logger.fine ("searching private peer group: " + groupName + ", searchUUID: " +
                          searchUUID + ", search string is: " + new String (param) + " (TTL: " +
                          peerSearchTTL + ")");

            LocalPrivatePeerGroupInfo lppgi = (LocalPrivatePeerGroupInfo) gi;
            String encryptedGroupName;
            byte[] encryptedParam;
            try {
                encryptedGroupName  = Base64Transcoders.convertByteArrayToB64String (
                        CryptoUtils.encryptStringUsingSecretKey (lppgi.key, groupName));
                encryptedParam = CryptoUtils.encryptUsingSecretKey (lppgi.key, param);
            }
            catch (Exception e) {
                throw new GroupManagerException ("Failed to search group <" + groupName + ">; nested exception: " + e);    
            }
            messageLen = MessageCodec.createPeerSearchPPGMessage (peerSearchMessage, _nodeUUID, getActiveNICsInfo(), 
                                                                  peerSearchTTL, searchUUID, _keyPair.getPublic(), groupName, 
                                                                  encryptedGroupName, param.length, encryptedParam);
       }
       else {
            throw new GroupManagerException ("Failed to search group <" + groupName + 
                                             ">: cannot initiate a peer search on a group I am not a member of");
       }

        // Store the peer search information
            if (peerSearchTTL < 0) {
                MessageCodec.updateTTLInPeerSearchMessage (peerSearchMessage, -1);
            }
            PeerSearchInfo psi = new PeerSearchInfo();
            psi.groupName = groupName;
            psi.searchUUID = searchUUID;
            psi.ttl = peerSearchTTL;
            psi.setMessage (peerSearchMessage, messageLen);
            psi.nodeUUID = getNodeUUID();
            psi.bPrivate = bPrivate;
            psi.hopCount = hopCount;
            psi.floodProb = floodProb;
            psi.publicKey = _keyPair.getPublic();
            psi.nicsInfo = null;
            _peerSearchInfos.put (searchUUID, psi);
            
            // Broadcast the message
            try {
                _nal.broadcastMessage (peerSearchMessage, messageLen, messageType, hopCount, floodProb, true);
            }
            catch (NetworkException ne) {
                throw new GroupManagerException ("Failed to search group <" + groupName + ">; nested exception: " + ne);
            }
            _lastPingTime = System.currentTimeMillis();

            return searchUUID;
        }

        /**
         * Start a search within the context of the specified group using the specified parameter
         * <p>
         * If the group name is a private group, the search request is encrypted using the password for the group
         *
         * @param groupName         name of the group
         * @param param             application defined parameter
         * @return                  returns an UUID that represents the search request
         * @throws GroupManagerException
         */
        public synchronized String startPeerSearch (String groupName, byte[] param)
            throws GroupManagerException
        {
            return startPeerSearch (groupName, param, _peerSearchHopCount, _peerSearchFloodProb, _peerSearchTTL);
        }

        public  synchronized String startPersistentPeerSearch (String groupName, byte[] param)
            throws GroupManagerException
        {
            return startPeerSearch (groupName, param, _peerSearchHopCount, _peerSearchFloodProb, -1);
        }

         /**
         * Stop a persistent search with the specified search UUID
         *
         * @param searchUUID    peer search UUID
         */
        public synchronized void stopPersistentPeerSearch (String searchUUID)
        {
            if (searchUUID != null) {
                PeerSearchInfo psi = (PeerSearchInfo) _peerSearchInfos.get (searchUUID);
                if (psi != null) {
                    _logger.info ("stopping persistent peer search: " + searchUUID);
                    _peerSearchInfos.remove (searchUUID);
                }
            }
        }

        /**
         * Respond to a peer search
         *
         * @param searchUUID    nodeUUID of the peer search
         * @param param         application defined parameter
         * @throws GroupManagerException
         */
        public synchronized void respondToPeerSearch (String searchUUID, byte[] param)
            throws GroupManagerException
        {
            PeerSearchInfo psi = (PeerSearchInfo) _peerSearchInfos.get (searchUUID);

            if (psi != null) {
                String groupName = psi.groupName;
                byte[] peerSearchReplyMessage = new byte [MessageCodec.MAX_MESSAGE_SIZE];
                int messageLen = 0;
                if ( ((_publicPeerGroups.get(groupName)) != null) ||
                     ((_privatePeerGroups.get(groupName)) != null)) {
                    byte[] parameter;
                    if (psi.publicKey != null) { // encode the reply
                        try {
                            parameter = CryptoUtils.encryptUsingPublicKey (psi.publicKey, param);
                        }
                        catch (InvalidKeyException ike) {
                            throw new GroupManagerException ("Cannot respond to peer search <" + searchUUID + ">; " +
                            "; nested exception - " + ike);
                        }
                    }
                    else {
                        parameter = param;
                    }
                    messageLen = MessageCodec.createPeerSearchReplyMessage (peerSearchReplyMessage, searchUUID, parameter);
                }
                else {
                    throw new GroupManagerException ("Cannot respond to peer search <" + psi.searchUUID +
                                                     "> - I am not a member of Peer Group <" + psi.groupName + ">");
                }

                // Respond to the node that started the peer search with a unicast UDP message
                // (PEER_SEACH_REPLY).
                _logger.fine ("responding to search UUID: " + searchUUID + " with string param: " +
                              new String (param) + " - sending unicast message to node <" + 
                              psi.nodeUUID + ">");
                try {
                    _nal.sendMessage (peerSearchReplyMessage, messageLen, psi.nicsInfo,
                                      MessageCodec.PEER_SEARCH_REPLY_MESSAGE, psi.ttl, true);
                }
                catch (NetworkException ne) {
                    throw new GroupManagerException ("Cannot respond to peer search <" + searchUUID + ">; " + 
                        "; nested exception - " + ne);
                }
            }
            else {
                throw new GroupManagerException ("Cannot respond to peer search <" + searchUUID + ">; " +
                        " invalid searchUUID");
            }
        }

        /**
         * Called by the application when its status changes. 
         * <p>
         * If any persistent peer searches are still active (therefore cached) the application
         * will be notified through the peerSearchReceived() callback for each of them.
         */
        public synchronized void statusChanged()
        {
            for (Enumeration e = _peerSearchInfos.elements(); e.hasMoreElements();) {
                PeerSearchInfo psi = (PeerSearchInfo) e.nextElement();
                if (psi.ttl > 0) {
                if (psi.getUpdatedTTL() <= 0) {
                    _peerSearchInfos.remove (psi.searchUUID);
                    continue;
                }
            }

            if (! psi.nodeUUID.equals (getNodeUUID())) { // I didn't initiate the persistent peer search
                handlePeerSearchMessage (psi.nodeUUID, psi.hopCount, psi.floodProb, 
                                         psi.bPrivate, psi.message, psi.messageLen, true);
            }
        }
    }

    /**
     * Not implemented yet
     *
     * @param groupName
     * @param param
     * @throws GroupManagerException
     */
    public synchronized void changeGroupMembershipParam (String groupName, String param)
        throws GroupManagerException
    {
        /*GroupInfo gi = (GroupInfo) _localGroups.get (groupName);
        if (gi == null) {
            throw new GroupManagerException ("not a member of group <" + groupName + ">");
        }
        gi.param = param; */
    }

    /**
     * Not implemented yet.
     *
     * @param groupName
     * @param mode
     * @throws GroupManagerException
     */
    public synchronized void changeGroupMembershipMode (String groupName, int mode)
        throws GroupManagerException
    {
        /*GroupInfo gi = (GroupInfo) _localGroups.get (groupName);
        if (gi == null) {
            throw new GroupManagerException ("not a member of group <" + groupName + ">");
        }
        gi.joinMode = mode; */
    }

    /**
     * Delete the specified group
     *
     * @param groupName      the name of the group to be deleted
     * @throws GroupManagerException
     */
    public synchronized void removeGroup (String groupName)
        throws GroupManagerException
    {
        if ((_publicManagedGroups.remove (groupName) == null) &&
            (_privateManagedGroups.remove (groupName) == null) &&
            (_publicPeerGroups.remove (groupName) == null) &&
            (_privatePeerGroups.remove (groupName) == null)) {
            throw new GroupManagerException ("could not find group <" + groupName + "> to remove");
        }
        updateInfoMessage();
        if (_running) {
            sendInfoMessage();
        }
        compileGroupList();
    }

    /**
     * Get the group UUID
     *
     * @return a string that represents the group nodeUUID
     */
    public String getNodeUUID()
    {
        return _nodeUUID;
    }

	/**
     * Checks whether a peer is alive or not 
     *
	 * @return <code>true</code> is the peer is alive
     */
	public boolean isPeerAlive (String nodeUUID)
	{
		return _peers.containsKey (nodeUUID);
	}

    /**
     * Get the peer info of the specified node given its UUID
     *
     * @param nodeUUID    the UUID of the node
     * @return    the peer group manager info
     */
    public PeerInfo getPeerInfo (String nodeUUID)
    {
        return (PeerInfo) _peers.get (nodeUUID);
    }
    
    /**
     * Get the name of the specified node given its UUID
     *
     * @param nodeUUID    the UUID of the node
     * @return            the name of the node
     */
    public String getPeerNodeName (String nodeUUID)
    {
        PeerInfo pi = getPeerInfo (nodeUUID);
        if (pi != null) {
            return pi.nodeName;
        }
        else {
            return null;
        }
    }

    /**
     * Get information about all groups, local and remote
     * <p>
     * The hashtable is mapping group names to GroupInfo objects.
     *
     * @return Hashtable<String, GroupInfo>
     */
    public Hashtable getGroups()
    {
        return _groups;
    }

    /**
     * Get the Local Public Peer Groups
     * <p>
     * The hashtable is mapping group names to LocalPublicPeerGroupInfo objects.
     *
     * @return Hashtable<String, LocalPublicPeerGroupInfo>
     */
    public Hashtable getLocalPublicPeerGroups()
    {
        return _publicPeerGroups;
    }

    /**
     * Get the Local Private Peer Groups
     * <p>
     * The hashtable is mapping group names to LocalPrivatePeerGroupInfo objects.
     *
     * @return Hashtable<LocalPrivatePeerGroupInfo>
     */
    public Hashtable getLocalPrivatePeerGroups()
    {
        return _privatePeerGroups;
    }

    /**
     * Get the local Public Managed Groups.
     * <p>
     * The hashtable is mapping group names to LocalPublicManagedGroupInfo objects.
     *
     * @return Hashtable<LocalPublicManagedGroupInfo>
     */
    public Hashtable getLocalPublicManagedGroups()
    {
        return _publicManagedGroups;
    }

    /**
     * Get the Local Private Managed Groups
     *
     * @return Hashtable<LocalPrivateManagedGroupInfo>
     */
    public Hashtable getLocalPrivateManagedGroups()
    {
        return _privateManagedGroups;
    }

    /**
     * Get the joined Public Managed Joined Groups
     * <p>
     * The hashtable is mapping group names to PublicManagedGroupJoinInfo objects
     *
     * @return Hashtable<PublicManagedGroupJoinInfo>
     */
    public Hashtable getJoinedPublicManagedGroups()
    {
        return _joinedPublicManagedGroups;
    }

    /**
     * Get the joined Private Managed Groups
     * <p>
     * The hashtable is mapping group names to PrivateManagedGroupJoinInfo objects
     *
     * @return Hashtable<PrivateManagedGroupJoinInfo>
     */
    public Hashtable getJoinedPrivateManagedGroups()
    {
        return _joinedPrivateManagedGroups;
    }

    /**
     * Get a local public group info for the specified group name.
     *
     * @param groupName      the name of the group
     * @return the local public group info.
     */
    public LocalPublicManagedGroupInfo getLocalPublicManagedGroupInfo (String groupName)
    {
        return (LocalPublicManagedGroupInfo) _publicManagedGroups.get (groupName);
    }

    /**
     * Get a local private group info for the specified group name and password.
     *
     * @param groupName      the name of the group
     * @return the local private group info.
     */
    public LocalPrivateManagedGroupInfo getLocalPrivateManagedGroupInfo (String groupName)
    {
        return (LocalPrivateManagedGroupInfo) _privateManagedGroups.get (groupName);
    }

    /**
     * Get a public peer group info for the specified group name.
     *
     * @param groupName      the name of the group
     * @return the public peer group info.
     */
    public LocalPublicPeerGroupInfo getLocalPublicPeerGroupInfo (String groupName)
    {
        return (LocalPublicPeerGroupInfo) _publicPeerGroups.get (groupName);
    }

    /**
     * Get a private peer group info for the specified group name.
     *
     * @param groupName      the name of the group
     * @return the private peer group info.
     */
    public LocalPrivatePeerGroupInfo getLocalPrivatePeerGroupInfo (String groupName)
    {
        return (LocalPrivatePeerGroupInfo) _privatePeerGroups.get (groupName);
    }

    /**
     * Get the name of the group
     *
     * @return a string indicating the name of the node
     */
    public String getNodeName()
    {
        return _nodeName;
    }

    /**
     * Set the name of the node
     *
     * @param nodeName
     */
    public void setNodeName (String nodeName)
    {
        if (nodeName != null) {
            _nodeName = nodeName;
            _logger.info ("nodeName set to: " + _nodeName);
            updateInfoMessage();
            if (_running) {
                sendInfoMessage();
            }
        }
    }
    
    /**
     * Set the node UUID
     * 
     * @param nodeUUID
     */
    public void setNodeUUID (String nodeUUID)
    {
        if (nodeUUID != null) {
            _nodeUUID = nodeUUID;
            _logger.info ("nodeUUID set to: " + _nodeUUID);
            updateInfoMessage();
            if (_running) {
                sendInfoMessage();
            }
        }
    }

    /**
     * Get the PING message send interval (default is 2 seconds)
     *
     * @return PING message send interval
     */
    public int getPingInterval()
    {
        return _pingInterval;
    }

    /**
     * Set the PING message send interval (default is 2 seconds)
     *
     * @param pingInterval  PING message send interval
     * @throws GroupManagerException
     */
    public void setPingInterval (int pingInterval)
        throws GroupManagerException
    {
        if (pingInterval == 0) {
            return;
        }
        else {
            _pingInterval = pingInterval;
        }
        updateInfoMessage();
        if (_running) {
            sendInfoMessage();
        }
    }

    /**
     * Set the INFO message send interval (default is 10 seconds)
     *
     * @param INFO message send interval
     */
    public void setInfoBCastInterval (int infoBCastInterval)
    {
        _infoBCastInterval = infoBCastInterval;
    }

    /**
     * Get the INFO message send interval (default is 10 seconds)
     *
     * @return INFO message send interval
     */
    public int getInfoBCastInterval()
    {
        return _infoBCastInterval;
    }

    /**
     * Set the node timeout factor. 
     * <p>
     * Sets the timeout factor (which is a multiple of a node's ping interval) for dropping a node
     * If the timeout factor is 3 and a node specifies a ping interval of 2000 ms, then the node
     * will be dropped if no PING packet is received from the node for 6000 ms.
     * <p>
     * Note that the actual timeout is based on the peer node's ping interval and hence can vary
     * for different nodes.
     *
     * @param nodeTimeoutFactor     node timeout factor
     */
    public void setNodeTimeoutFactor (byte nodeTimeoutFactor)
    {
        if (nodeTimeoutFactor < 2) {
            return;
        }
        _nodeTimeoutFactor = nodeTimeoutFactor;
    }

    /**
     * Get the node timeout factor
     * <p>
     * Sets the timeout factor (which is a multiple of a node's ping interval) for dropping a node
     * If the timeout factor is 3 and a node specifies a ping interval of 2000 ms, then the node
     * will be dropped if no PING packet is received from the node for 6000 ms.
     * <p>
     * Note that the actual timeout is based on the peer node's ping interval and hence can vary
     * for different nodes.
     *
     * @return node timeout factor
     */
    public int getNodeTimeoutFactor()
    {
        return _nodeTimeoutFactor;
    }

    /**
     * Get the default peer search time to live (default is 0)
     * <p>
     * If the TTL != 0, the Peer Search packets will be rebroadcasted every 
     * _peerSearchResendInterval, until the TTL will elapse. At this point the 
     * Peer Search information will be discarded.
     * <p>
     * Each node receiving a Persistent Peer Search Message (i.e. with TTL != 0) will
     * cache it, and will unicast to every new node that will become visible.
     * <p>  
     * If TTL is lower then 0, the Peer Search will persist until the stopPeerSearch() method will
     * be called in the node that originated it. The node originating the Persistent
     * Peer Search will then discard it, and each node that has cached it will discard
     * it as well after _ui32MaxPeerSearchLeaseTime (or when the TTL will elapse).
     * <p>
     * A Persistent Peer Search Message (i.e. with TTL != 0) will not be cached for
     * more than _maxPeerSearchLeaseTime (this value is hardcoded and equal in
     * all nodes). The node that originated the persistent peer search resends 
     * periodically the peer seach packet in order to keep the search active.
     *
     * @return default persistent peer search TTL
     */
    public int getPeerSearchTTL()
    {
        return _peerSearchTTL;
    }

    /**
     * Set the default peer search time to live (default is 0)
     * <p>
     * If the TTL != 0, the Peer Search packets will be rebroadcasted every
     * _peerSearchResendInterval, until the TTL will elapse. At this point the
     * Peer Search information will be discarded.
     * <p>
     * Each node receiving a Persistent Peer Search Message (i.e. with TTL != 0) will
     * cache it, and will unicast to every new node that will become visible.
     * <p>
     * If TTL lower than 0, the Peer Search will persist until the stopPeerSearch() method will
     * be called in the node that originated it. The node originating the Persistent
     * Peer Search will then discard it, and each node that has cached it will discard
     * it as well after _ui32MaxPeerSearchLeaseTime (or when the TTL will elapse).
     * <p>
     * A Persistent Peer Search Message (i.e. with TTL != 0) will not be cached for
     * more than _maxPeerSearchLeaseTime (this value is hardcoded and equal in
     * all nodes). The node that originated the persistent peer search resends
     * periodically the peer seach packet in order to keep the search active.
     *
     * @param peerSearchTTL default persistent peer search TTL
     */
    public void setPeerSearchTTL (int peerSearchTTL)
    {
        _peerSearchTTL = peerSearchTTL;
    }

    /**
     * Set the peer search resend interval
     * <p>
     * This is the amount of time after which a Persistent Peer Search packet 
     * (i.e. a Peer Search with TTL!=0) is rebroadcasted. 
     * <p>
     * This value has to be lower of the maximum Peer Search leasing time. If the 
     * value is greater than the Maximum Peer Search Lease Time 
     * (DEFAULT_MAX_PEER_SEARCH_LEASE_TIME), an exception is thrown.
     *
     * @param peerSearchResendInterval  peer search resend interval
     * @throws GroupManagerException
     */
    public void setPeerSearchResendInterval (int peerSearchResendInterval)
        throws GroupManagerException
    {
        if (peerSearchResendInterval > PEER_SEARCH_LEASE_TIME) {
            throw new GroupManagerException ("Invalid value. Has to be lower than DEFAULT_MAX_PEER_SEARCH_LEASE_TIME = " + 
                                             PEER_SEARCH_LEASE_TIME);
        }
        _peerSearchResendInterval = peerSearchResendInterval;
    }

    /**
     * Get the peer search resend interval
     * <p>
     * This is the amount of time after which a Persistent Peer Search packet
     * (i.e. a Peer Search with TTL!=0) is rebroadcasted.
     * <p>
     * This value has to be lower of the maximum Peer Search leasing time.
     *
     * @return  peer search resend interval
     */
    public int getPeerSearchResendInterval()
    {
        return _peerSearchResendInterval;
    }

    /**
     * Gets the maximum persistent peer search lease time. 
     * <p>
     * This value is hardcoded since it has to be the same for all the nodes. 
     * After this time is elapsed the Persistent Peer Search is discarded by 
     * the node. This value has to be greater than the Peer Search Resend 
     * Interval.
     *
     * @return maximum persistent peer search lease time
     */
    public int getMaxPeerSearchLeaseTime()
    {
        return _maxPeerSearchLeaseTime;
    }

    /**
     * Set the default number of hops that PING packets are propagated for.
     * (default is 1)
     *
     * @param pingHopCount    the ping hop count
     */
    public void setPingHopCount (int pingHopCount)
    {
        _pingHopCount = pingHopCount;
    }

    /**
     * Get the default number of hops that PING packets are propagated for.
     * (default is 1)
     *
     * @return  a default of 1 if the ping hop count is not set
     */
    public int getPingHopCount()
    {
        return _pingHopCount;
    }

    /**
     * Set the default peer search hop count (default is 1)
     *
     * @param peerSearchHopCount    default peer search hop count
     */
    public void setPeerSearchHopCount (int peerSearchHopCount)
    {
        _peerSearchHopCount = peerSearchHopCount;
    }

    /**
     * Get the default peer search hop count (default is 1)
     *
     * @return default peer search hop count
     */
    public int getPeerSearchHopCount()
    {
        return _peerSearchHopCount;
    }

    /**
     * Set the default PING flood probability (default is 100)
     *
     * @param pingFloodProb    default ping flood probability
     */
    public void setPingFloodProbability (int pingFloodProb)
    {
        _pingFloodProb = pingFloodProb;
    }

    /**
     * Get the default PING flood probability (default is 100)
     *
     * @return default ping flood probability
     */
    public int getPingFloodProbability()
    {
        return _pingFloodProb;
    }

    /**
     * Set the default peer search flood probability (default is 100)
     *
     * @param peerSearchFloodProb    peer search flood probability
     */
    public void setPeerSearchFloodProbability (int peerSearchFloodProb)
    {
        _peerSearchFloodProb = peerSearchFloodProb;
    }

    /**
     * Get the peer search flood probability (default is 100)
     *
     * @return peer search flood probability
     */
    public int getPeerSearchFloodProbability()
    {
        return _peerSearchFloodProb;
    }

    /**
     * Get information about the Network Interfaces used by the
     * Group Manager.
     * <p>
     * @see us.ihmc.net.NICInfo
     *
     * @return Vector<NICInfo>
     */
    public Vector getActiveNICsInfo()
    {
        return _nal.getActiveNICsInfo();
    }
    
    /**
     * 
     * 
     * @param relayNodes - Vector<InetAddress>
     */
    public void setRelayNodes (Vector relayNodes)
    {
    	_nal.setRelayNodes (relayNodes);
    }
        
    /**
     * Prints to the STDOUT information about the current state of 
     * the Group Manager.
     */
    public void dumpState()
    {
        System.out.println ("Dumping state of Group Manager at node: " + _nodeUUID);
        System.out.println ("******************************************************************\n");
        System.out.println ("Local Public Managed Groups");
        System.out.println ("---------------------------");
        Enumeration publicManagedGroups = _publicManagedGroups.keys();
        while (publicManagedGroups.hasMoreElements()) {
            String groupName = (String) publicManagedGroups.nextElement();
            System.out.println ("Group Name: " + groupName);
            System.out.print ("  Members: ");
            Enumeration members = ((GroupInfo) _publicManagedGroups.get (groupName)).members.elements();
            while (members.hasMoreElements()) {
                System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
            }
            System.out.println();
        }

        System.out.println ("Local Private Managed Groups");
        System.out.println ("----------------------------");
        Enumeration privateManagedGroups = _privateManagedGroups.keys();
        while (privateManagedGroups.hasMoreElements()) {
            String groupName = (String) privateManagedGroups.nextElement();
            System.out.println ("Group Name: " + groupName);
            System.out.print ("  Members: ");
            Enumeration members = ((GroupInfo) _privateManagedGroups.get (groupName)).members.elements();
            while (members.hasMoreElements()) {
                System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
            }
            System.out.println();
        }

        System.out.println ("Local Public Peer Groups");
        System.out.println ("------------------------");
        Enumeration publicPeerGroups = _publicPeerGroups.keys();
        while (publicPeerGroups.hasMoreElements()) {
            String groupName = (String) publicPeerGroups.nextElement();
            System.out.println ("Group Name: " + groupName);
            System.out.print ("  Members: ");
            Enumeration members = ((GroupInfo) _publicPeerGroups.get (groupName)).members.elements();
            while (members.hasMoreElements()) {
                System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
            }
            System.out.println();
        }

        System.out.println ("Local Private Peer Groups");
        System.out.println ("-------------------------");
        Enumeration privatePeerGroups = _privatePeerGroups.keys();
        while (privatePeerGroups.hasMoreElements()) {
            String groupName = (String) privatePeerGroups.nextElement();
            System.out.println ("Group Name: " + groupName);
            System.out.print ("  Members: ");
            Enumeration members = ((GroupInfo) _privatePeerGroups.get (groupName)).members.elements();
            while (members.hasMoreElements()) {
                System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
            }
            System.out.println();
        }

        System.out.println();
        System.out.println ("Peer Node List");
        System.out.println ("--------------");
        Enumeration peers = _peers.elements();
        while (peers.hasMoreElements()) {
            PeerInfo pi = (PeerInfo) peers.nextElement();
            System.out.println ("Peer Group Manager - " + pi.nodeUUID + ", " + pi.nodeName);
        }

        System.out.println();
        System.out.println ("Group List");
        System.out.println ("----------");
        Enumeration groups = _groups.elements();
        while (groups.hasMoreElements()) {
            GroupInfo gi = (GroupInfo) groups.nextElement();
            if (gi instanceof LocalPublicManagedGroupInfo) {
                LocalPublicManagedGroupInfo lpmgi = (LocalPublicManagedGroupInfo) gi;
                System.out.print ("Local public managed group - " + lpmgi.groupName + "; members: ");
                Enumeration members = lpmgi.members.elements();
                while (members.hasMoreElements()) {
                    System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
                }
                System.out.println();
            }
            else if (gi instanceof LocalPrivateManagedGroupInfo) {
                LocalPrivateManagedGroupInfo lpmgi = (LocalPrivateManagedGroupInfo) gi;
                System.out.print ("Local private managed group - " + lpmgi.groupName + "; members: ");
                Enumeration members = lpmgi.members.elements();
                while (members.hasMoreElements()) {
                    System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
                }
                System.out.println();
            }
            else if (gi instanceof LocalPublicPeerGroupInfo) {
                LocalPublicPeerGroupInfo lppgi = (LocalPublicPeerGroupInfo) gi;
                System.out.print ("Local public peer group - " + lppgi.groupName + "; members: ");
                Enumeration members = lppgi.members.elements();
                while (members.hasMoreElements()) {
                    System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
                }
                System.out.println();
            }
            else if (gi instanceof LocalPrivatePeerGroupInfo) {
                LocalPrivatePeerGroupInfo lppgi = (LocalPrivatePeerGroupInfo) gi;
                System.out.print ("Local private peer group - " + lppgi.groupName + "; members: ");
                Enumeration members = lppgi.members.elements();
                while (members.hasMoreElements()) {
                    System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
                }
                System.out.println();
            }
            else if (gi instanceof RemotePublicManagedGroupInfo) {
                RemotePublicManagedGroupInfo rpmgi = (RemotePublicManagedGroupInfo) gi;
                System.out.print ("Remote public managed group - " + rpmgi.groupName + " - from node " +
                                  rpmgi.creatorUUID + "; members: ");
                Enumeration members = rpmgi.members.elements();
                while (members.hasMoreElements()) {
                    System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
                }
                System.out.println();
            }
            else if (gi instanceof RemotePrivateManagedGroupInfo) {
                RemotePrivateManagedGroupInfo rpmgi = (RemotePrivateManagedGroupInfo) gi;
                System.out.print ("Remote private managed group - " + rpmgi.groupName + " - from node " +
                                  rpmgi.creatorUUID + "; members: ");
                Enumeration members = rpmgi.members.elements();
                while (members.hasMoreElements()) {
                    System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
                }
                System.out.println();
            }
            else if (gi instanceof RemotePublicPeerGroupInfo) {
                RemotePublicPeerGroupInfo rppgi = (RemotePublicPeerGroupInfo) gi;
                System.out.print ("Remote public peer group - " + rppgi.groupName + " - from node " +
                                  rppgi.creatorUUID + "; members: ");
                Enumeration members = rppgi.members.elements();
                while (members.hasMoreElements()) {
                    System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
                }
                System.out.println();
            }
            else if (gi instanceof RemotePrivatePeerGroupInfo) {
                RemotePrivatePeerGroupInfo rppgi = (RemotePrivatePeerGroupInfo) gi;
                System.out.print ("Remote private peer group - " + rppgi.groupName + " - from node " +
                                  rppgi.creatorUUID + "; members: ");
                Enumeration members = rppgi.members.elements();
                while (members.hasMoreElements()) {
                    System.out.print (((GroupMemberInfo) members.nextElement()).nodeUUID + " ");
                }
                System.out.println();
            }
            else {
                System.out.println ("Group of unknown type - " + gi.groupName);
            }
        }
        System.out.println ("******** DONE ********\n\n");
    }

    ///////////////////// Protected methods /////////////////////

    /**
     * Handle a message. 
     * <p>
     * This method is called by the NetworkAccessLayer component
     * after unwrapping the message from a received packet.
     *
     * @param nodeUUID
     * @param messageType
     * @param hopCount
     * @param floodProb
     * @param nicsInfo Vector<NICInfo>
     * @param message
     * @param messageLen
     * @return
     */
    protected int handleMessage (String nodeUUID, int messageType, int hopCount,
                                 int floodProb, Vector nicsInfo, byte[] message,
                                 int messageLen)
    {
        if (!MessageCodec.isMessageValid (message, messageLen, messageType)) {
            _logger.warning ("invalid message received of type: " + messageType);
            return -1;
        }

        // Update the last contact time (each message counts as a PING message)
        PeerInfo pi = (PeerInfo) _peers.get (nodeUUID);
        if (pi != null) {
            pi.lastContactTime = System.currentTimeMillis();
            pi.pingCount++;
        }

        // Process the message based on its type
        switch (messageType)
        {
            case MessageCodec.PING_MESSAGE:
                handlePingMessage (nodeUUID, message, messageLen);
                break;
            case MessageCodec.INFO_MESSAGE:
                handleInfoMessage (nodeUUID, message, messageLen, nicsInfo);
                break;
            case MessageCodec.PEER_SEARCH_MESSAGE:
                handlePeerSearchMessage (nodeUUID, hopCount, floodProb, false, message, messageLen, false);
                break;
            case MessageCodec.PEER_SEARCH_PPG_MESSAGE:
                handlePeerSearchMessage (nodeUUID, hopCount, floodProb, true, message, messageLen, false);
                break;
            case MessageCodec.PEER_SEARCH_REPLY_MESSAGE:
                handlePeerSearchReplyMessage (nodeUUID, message, messageLen);
                break;
            case MessageCodec.GROUP_DATA_MESSAGE:
                handlePeerGroupDataMessage (nodeUUID, message, messageLen);
                break;
            default:
                // Unknown message type received - ignore it
                _logger.warning ("received unknown message of type: " + messageType);
        }

        return 0;
    }

    /**
     * Handle a Join Public Managed Group request (received via TCP)
     *
     * @param addr
     * @param port
     * @param nodeUUID
     * @param groupName
     * @param joinData
     * @param pubKey
     * @return
     */
    protected synchronized boolean handleJoinPublicManagedGroup (InetAddress addr, int port, String nodeUUID,
                                                                 String groupName, byte[] joinData, PublicKey pubKey)
    {
        LocalPublicManagedGroupInfo pgi = (LocalPublicManagedGroupInfo) _publicManagedGroups.get (groupName);
        if (pgi == null) {
        	_logger.info ("PublicManagedGroup join request failed - <" + groupName + ">: no such group exists locally");
            return false;
        }

        if (getPeerInfo (nodeUUID) == null) {
        	_logger.info ("PublicManagedGroup join request failed - don't know anything about peer <" + nodeUUID + ">");
            return false;
        }

        PublicManagedGroupMemberInfo pi = (PublicManagedGroupMemberInfo) pgi.members.get (nodeUUID);
        if (pi == null) {
            // This is the first time the node is joining the group
            pi = new PublicManagedGroupMemberInfo();
            pgi.members.put (nodeUUID, pi);
        }

        // Set (or update) the values
        pi.nodeUUID = nodeUUID;
        pi.addr = addr;
        pi.port = port;
        pi.data = (joinData != null) ? (byte[]) joinData.clone() : null;
        pi.pubKey = pubKey;

        if (_listener != null) {
            _listener.newGroupMember (groupName, nodeUUID, joinData);
        }

        return true;
    }

    /**
     * Handle a Join Private Managed Group request (received via TCP)
     *
     * @param addr
     * @param port
     * @param nodeUUID
     * @param groupName
     * @param joinData
     * @param pubKey
     * @param nonce
     * @return
     */
    protected synchronized boolean handleJoinPrivateManagedGroup (InetAddress addr, int port, String nodeUUID,
                                                                  String groupName, byte[] joinData, PublicKey pubKey,
                                                                  String nonce)
    {
        LocalPrivateManagedGroupInfo rgi = (LocalPrivateManagedGroupInfo) _privateManagedGroups.get (groupName);
        if (rgi == null) {
        	_logger.info ("PrivateManagedGroup join request failed - <" + groupName + ">: no such group exists locally");
        	return false;
        }

        if (getPeerInfo (nodeUUID) == null) {
        	_logger.info ("PrivateManagedGroup join request failed - don't know anything about peer <" + nodeUUID + ">");
        	return false;
        }

        if (!rgi.nonce.equals (nonce)) {
        	_logger.info ("PrivateManagedGroup join request failed - remote side did not use the correct password");
            return false;
        }

        PrivateManagedGroupMemberInfo rgmi = (PrivateManagedGroupMemberInfo) rgi.members.get (nodeUUID);
        if (rgmi == null) {
            // This is the first time the node is joining the group
            rgmi = new PrivateManagedGroupMemberInfo();
            rgi.members.put (nodeUUID, rgmi);
        }

        // Set (or update) the values
        rgmi.nodeUUID = nodeUUID;
        rgmi.addr = addr;
        rgmi.port = port;
        rgmi.data = (joinData != null) ? (byte[]) joinData.clone() : null;
        rgmi.pubKey = pubKey;

        if (_listener != null) {
            _listener.newGroupMember (groupName, nodeUUID, joinData);
        }

        return true;
    }

    /**
     * Handle a Leave Managed Group request (received via TCP)
     *
     * @param addr
     * @param port
     * @param nodeUUID
     * @param groupName
     * @return
     */
    protected synchronized boolean handleLeaveGroup (InetAddress addr, int port, String nodeUUID, String groupName)
    {
        LocalPublicManagedGroupInfo pgi = (LocalPublicManagedGroupInfo) _publicManagedGroups.get (groupName);
        if (pgi != null) {
            PublicManagedGroupMemberInfo pi = (PublicManagedGroupMemberInfo) pgi.members.remove (nodeUUID);
            if (pi == null) {
                // The remote group manager is not a member of this group
                return false;
            }
            if (_listener != null) {
                _listener.groupMemberLeft (groupName, nodeUUID);
            }
            return true;
        }
        LocalPrivateManagedGroupInfo rgi = (LocalPrivateManagedGroupInfo) _privateManagedGroups.get (groupName);
        if (rgi != null) {
            PrivateManagedGroupMemberInfo rgmi = (PrivateManagedGroupMemberInfo) rgi.members.remove (nodeUUID);
            if (rgmi == null) {
                // The remote group manager is not a member of this group
                return false;
            }
            if (_listener != null) {
                _listener.groupMemberLeft (groupName, nodeUUID);
            }
            return true;
        }

        // The group was not found in the list of public or private groups
        return false;
    }

    ///////////////////// Private Methods /////////////////////

    /**
     * Initialized the Group Manager
     *
     * @param port
     * @param netIFsConf Vector<NetworkInterfaceConf>
     * @throws GroupManagerException
     */
    private void firstTimeInit (int port, Vector netIFsConf)
        throws GroupManagerException
    {
        try {
            CryptoUtils.setProvider ("BC");
        }
        catch (Exception e) {
            throw new GroupManagerException ("Could not find BouncyCastle provider for JCE; nested exception - " + e);
        }
        _logger = LogHelper.getLogger ("GroupManager");
        _port = port;
        _infoStateSeqNo = _groupDataStateSeqNo = (int) ((System.currentTimeMillis() / 1000) % MAX_SEQUENCE_NUM);
        if (_nodeUUID == null) {
            _nodeUUID = IDGenerator.generateID();
        }
        _nodeName = NetUtils.getLocalHostName();
        _peers = new Hashtable();
        _deadPeers = new Hashtable();
        _groups = new Hashtable();

        _logger.info ("nodeName: " + _nodeName);
        _logger.info ("nodeUUID: " + _nodeUUID);

        // Initializing the Network Access Layer
        _nal = new NetworkAccessLayer();
        try {
            _nal.init (_nodeUUID, _port, netIFsConf, this);
        }
        catch (NetworkException ne) {
            throw new GroupManagerException ("Could not initialize NetworkAccessLayer; nested exception - " + ne);
        }

        _keyPair = CryptoUtils.generateNewKeyPair();

        // Initialize the TCP Server (used for manager groups)
        _tcpServer = new TCPServer();
        try {
            _tcpServer.init (_nodeUUID, _port, this, _keyPair);
        }
        catch (NetworkException ne) {
           throw new GroupManagerException ("Could not initialize TCPServer; nested exception - " + ne);
        }
    }

    /**
     * Handle a PING message (received via UDP)
     *
     * @param nodeUUID
     * @param message
     * @param messageLen
     */
    private synchronized void handlePingMessage (String nodeUUID, byte[] message, int messageLen)
    {
        PeerInfo pi = (PeerInfo) _peers.get (nodeUUID);
        if (pi == null) {
            // Check if we already know this node
            pi = (PeerInfo) _deadPeers.get (nodeUUID);
            if (pi != null) {
                 pi.lastContactTime = System.currentTimeMillis();
                _peers.put (nodeUUID, pi);
                _deadPeers.remove (nodeUUID);

                // Notify the listener (if any) about the rebirth of this peer
                if (_listener != null) {
                    _listener.newPeer (nodeUUID);
                }
            }

            // The ping message is from a node that is not known - send an INFO message
            sendInfoMessage();
        }
    }

    /**
     * Handle a INFO message (received via UDP)
     *
     * @param nodeUUID
     * @param message
     * @param messageLen
     * @param nicsInfo Vector<NICInfo>
     */
    private synchronized void handleInfoMessage (String nodeUUID, byte[] message, int messageLen, Vector nicsInfo)
    {
        long currentTime = System.currentTimeMillis();
        boolean updatePeerInfo = true;
        PeerInfo pi = (PeerInfo) _peers.get (nodeUUID);
        if (pi == null) {
            // Check if we already know this node
            pi = (PeerInfo) _deadPeers.get (nodeUUID);
            if (pi != null) {
                pi.lastContactTime = currentTime;
                _deadPeers.remove (nodeUUID);
                _peers.put (nodeUUID, pi);
            }
            else {
                // we have never seen this node before
                updatePeerInfo = false;
                pi = new PeerInfo();
                pi.nodeUUID = nodeUUID;
                pi.nicsInfo = nicsInfo;
                pi.stateSeqNo = MessageCodec.getStateSeqNoFromInfoMessage (message);  //stateSeqNo;
                pi.grpDataStateSeqNo = -1;
                pi.pingInterval = MessageCodec.getPingIntervalFromInfoMessage (message);  //pingInterval;
                pi.nodeName = MessageCodec.getNodeNameFromInfoMessage (message);  //nodeName;
                pi.pubKey = MessageCodec.getPublicKeyFromInfoMessage (message); //pubKey;
                pi.lastContactTime = pi.pingCountResetTime = currentTime;
                pi.pingCount = 1;
                pi.groups = MessageCodec.getGroupListFromInfoMessage (nodeUUID, message);    //groupList;
                pi.addr = NetUtils.determineDestIPAddr (pi.nicsInfo, _nal.getActiveNICsInfo());
                pi.port = _port;
                _peers.put (nodeUUID, pi);

                // Send an info packet so that the new peer node can get information about this node
                sendInfoMessage();
            }

            compileGroupList();
            if (_listener != null) {
                _listener.newPeer (nodeUUID);
            }

            // As soon as a new node is "visible" the active Persistent Peer Searches are
            // forwarded (via unicast) to the node
            forwardPersistentPeerSearchMessages (nodeUUID);

            // Check if any of the remote groups are peers of local peer groups - if so, update the membership
            for (Enumeration en = pi.groups.elements(); en.hasMoreElements();) {
                RemoteGroupInfo rgi = (RemoteGroupInfo) en.nextElement();
                if (rgi.groupName != null) {
                    // check if we are part of the same Public Peer Group
                    LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) _publicPeerGroups.get (rgi.groupName);
                    if (ppgi != null) {
                        if (!(rgi instanceof RemotePublicPeerGroupInfo)) {
                            if (_listener != null) {
                                _listener.conflictWithPrivatePeerGroup (ppgi.groupName, nodeUUID);
                            }
                        }
                        else {
                            addRemotePublicPeerGroup (ppgi, (RemotePublicPeerGroupInfo) rgi);
                        }
                    }

                    // check if we are part of the same Private Peer Group
                    LocalPrivatePeerGroupInfo lppgi = (LocalPrivatePeerGroupInfo) _privatePeerGroups.get (rgi.groupName);
                    if (lppgi != null) {
                        if (!(rgi instanceof RemotePrivatePeerGroupInfo)) {
                            if (_listener != null) {
                                _listener.conflictWithPrivatePeerGroup (lppgi.groupName, nodeUUID);
                            }
                        }
                        else {
                            addRemotePrivatePeerGroup (lppgi, (RemotePrivatePeerGroupInfo) rgi);
                        }
                    }
                }
            }
        }
        
        if (updatePeerInfo) {
            int stateSeqNo = MessageCodec.getStateSeqNoFromInfoMessage (message);
            if ((stateSeqNo > pi.stateSeqNo) || (stateSeqNo < (pi.stateSeqNo - MAX_SEQUENCE_NUM/2))) {
                int pingInterval = MessageCodec.getPingIntervalFromInfoMessage (message);
                String nodeName = MessageCodec.getNodeNameFromInfoMessage (message);
                PublicKey pubKey = MessageCodec.getPublicKeyFromInfoMessage (message);
                Hashtable groupList = MessageCodec.getGroupListFromInfoMessage (nodeUUID, message);
                
                if (pi.pingInterval != pingInterval) {
                    // ping message interval has changed
                    pi.pingInterval = pingInterval;
                }
                if (!pi.nodeName.equals (nodeName)) {
                    // Node name has changed
                    pi.nodeName = nodeName;
                }
                // Check if any of deleted remote groups were peers of local peer groups - if so, update the membership
                // because the remote group manager is no longer a member of the local peer group
                Hashtable addedGroups = new Hashtable();
                Hashtable deletedGroups = new Hashtable();
                if (Diff.hashtableDiff (pi.groups, groupList, null, deletedGroups, addedGroups)) {
                    pi.groups = groupList;
                    compileGroupList();

                    // Check if any of the added remote groups were peers of local peer groups - if so, update the membership
                    for (Enumeration en = addedGroups.elements(); en.hasMoreElements();) {
                        RemoteGroupInfo rgi = (RemoteGroupInfo) en.nextElement();
                        LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) _publicPeerGroups.get (rgi.groupName);
                        if (ppgi != null) {
                            if (! (rgi instanceof RemotePublicPeerGroupInfo)) {
                                if (_listener != null) {
                                    _listener.conflictWithPrivatePeerGroup (ppgi.groupName, nodeUUID);
                                }
                            }
                            else {
                                addRemotePublicPeerGroup (ppgi, (RemotePublicPeerGroupInfo) rgi);
                            }
                        }
                        LocalPrivatePeerGroupInfo lppgi = (LocalPrivatePeerGroupInfo) _privatePeerGroups.get (rgi.groupName);
                        if (lppgi != null) {
                            if (! (rgi instanceof RemotePrivatePeerGroupInfo)) {
                                if (_listener != null) {
                                    _listener.conflictWithPrivatePeerGroup (lppgi.groupName, nodeUUID);
                                }
                            }
                            else {
                                addRemotePrivatePeerGroup (lppgi, (RemotePrivatePeerGroupInfo) rgi);
                            }
                        }
                    }

                    // Check if any of deleted remote groups were peers of local peer groups - if so, update the membership
                    // because the remote group manager is no longer a member of the local peer group
                    for (Enumeration en = deletedGroups.keys(); en.hasMoreElements();) {
                        String deletedGroupName = (String) en.nextElement();
                        LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) _publicPeerGroups.get (deletedGroupName);
                        if ((ppgi != null) && (ppgi.members.remove (pi.nodeUUID) != null)) {
                            if (_listener != null) {
                                _listener.groupMemberLeft (deletedGroupName, pi.nodeUUID);
                            }
                        }
                        LocalPrivatePeerGroupInfo lppgi = (LocalPrivatePeerGroupInfo) _privatePeerGroups.get (deletedGroupName);
                        if ((lppgi != null) && (lppgi.members.remove (pi.nodeUUID) != null)) {
                            if (_listener != null) {
                                _listener.groupMemberLeft (deletedGroupName, pi.nodeUUID);
                            }
                        }
                    }
                    if (_listener != null) {
                        _listener.groupListChange (nodeUUID);
                    }
                }
                pi.stateSeqNo = stateSeqNo;
            }
        }
    }

    /**
     * Handle a PEER_SEARCH_MESSAGE/PEER_SEARCH_PPG message (received via UDP)
     *
     * @param nodeUUID
     * @param hopCount
     * @param floodProb
     * @param nicsInfo
     * @param bPrivate
     * @param message
     * @param messageLen
     * @param processMessage
     */
    private synchronized void handlePeerSearchMessage (String nodeUUID, int hopCount, int floodProb,
                                                       boolean bPrivate, byte[] message,
                                                       int messageLen, boolean processMessage)
    {
        int ttl;
        String origNodeUUID; // UUID of the node originating the peer search (not necessarily = nodeUUID)
        String searchUUID;
        String groupName;
        PublicKey publicKey;
        Vector nicsInfo;

        if (bPrivate) {
            origNodeUUID = MessageCodec.getNodeUUIDFromPeerSearchPPGMessage (message);
            ttl = MessageCodec.getTTLFromPeerSearchPPGMessage (message);
            searchUUID = MessageCodec.getSearchUUIDFromPeerSearchPPGMessage (message);
            groupName = MessageCodec.getGroupNameFromPeerSearchPPGMessage (message);
            publicKey = MessageCodec.getPublicKeyFromPeerSearchPPGMessage (message);
            nicsInfo = MessageCodec.getNICsInfoFromPeerSearchPPGMessage (message);
        }
        else {
            origNodeUUID = MessageCodec.getNodeUUIDFromPeerSearchMessage (message);
            ttl = MessageCodec.getTTLFromPeerSearchMessage (message);
            searchUUID = MessageCodec.getSearchUUIDFromPeerSearchMessage (message);
            groupName = MessageCodec.getGroupNameFromPeerSearchMessage (message);
            publicKey = MessageCodec.getPublicKeyFromPeerSearchMessage (message);
            nicsInfo = MessageCodec.getNICsInfoFromPeerSearchPPGMessage (message);
        }

        // Cache the Peer Search if this is the time we receive the packet
        PeerSearchInfo psi = (PeerSearchInfo) _peerSearchInfos.get (searchUUID);
        if (psi == null) {
            psi = new PeerSearchInfo();
            psi.groupName = groupName;
            psi.searchUUID = searchUUID;
            psi.ttl = ttl;
            psi.setMessage (message, messageLen);
            psi.nodeUUID = origNodeUUID;
            psi.bPrivate = bPrivate;
            psi.hopCount = hopCount - 1;
            psi.floodProb = floodProb;
            psi.publicKey = publicKey;
            psi.nicsInfo = nicsInfo;
            _peerSearchInfos.put (searchUUID, psi);
            processMessage = true;
        }
        else {
            psi.lastRxTime = System.currentTimeMillis();
        }

        if (processMessage) {
            GroupInfo gi = null;
            if ((!bPrivate) && ((gi = (GroupInfo) _publicPeerGroups.get (groupName))!= null)) {
                // This is a PEER_SEARCH_MESSAGE
                LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) gi;
                // There is a local group with the same name - see if it has the peer node as a member
                if (ppgi.members.get (origNodeUUID) != null) {
                    // Notify the listener (if any) of the received search request
                    byte[] param = MessageCodec.getParamFromPeerSearchMessage (message);
                    if (_listener != null) {
                        _listener.peerSearchRequestReceived (groupName, origNodeUUID, searchUUID, param);
                    }
                }
            }
            else if ((bPrivate) && ((gi = (GroupInfo) _privatePeerGroups.get (groupName)) != null)) {
                // This is a PEER_SEARCH_PPG_MESSAGE
                LocalPrivatePeerGroupInfo lppgi = (LocalPrivatePeerGroupInfo) gi;
                String encryptedGroupName = MessageCodec.getEncryptedGroupNameFromPeerSearchPPGMessage (message);
                // There is a local group with the same name - see if it has the peer node as a member
                if (lppgi.members.get (origNodeUUID) != null) {
                    // The peer node is a member of the local private peer group, which means that the
                    // passwords must match
                    if (_listener != null) {
                        // Check if the Group Name and the Encrypted Group Name match (i.e. we have the correct
                        // key for decrypting the message).
                        try {
                            if (encryptedGroupName.equals (lppgi.encryptedGroupName)) {
                                // Get the length of the param
                                byte[] encryptedParam = MessageCodec.getEncryptedParamFromPeerSearchPPGMessage (message);
                                int unencryptedParamLen = MessageCodec.getUnencryptedParamLenFromPeerSearchRPGMessage (message);
                                byte[] param = CryptoUtils.decryptUsingSecretKey (lppgi.key, encryptedParam);
                                byte[] choppedParam = new byte[unencryptedParamLen];
                                System.arraycopy (param, 0, choppedParam, 0, unencryptedParamLen);
                                if (param != null) {
                                    _listener.peerSearchRequestReceived (groupName, origNodeUUID, searchUUID, choppedParam);
                                }
                            }
                            else {
                                _logger.warning ("unable to decrypt and cannot notify listener");
                            }
                        }
                        catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
        }
    }

    /**
     * Handle a PEER_SEARCH_REPLY message (received via UDP)
     *
     * @param nodeUUID       peer nodeUUID
     * @param message        search peer message byte array
     * @param messageLen     message length
     */
    private synchronized void handlePeerSearchReplyMessage (String nodeUUID, byte[] message, int messageLen)
    {
        String searchUUID = MessageCodec.getSearchUUIDFromPeerSearchReplyMessage (message);
        String groupName = null;

        PeerSearchInfo psi = (PeerSearchInfo) _peerSearchInfos.get (searchUUID);
        if (psi != null) {
            groupName = psi.groupName;
        }
        else {
            _logger.fine ("received a PEER_SEARCH_REPLY_MESSAGE for a peer search that I did not initiate!");
            return;
        }

        if ((_listener != null) && (groupName != null)) {
            if ((_publicPeerGroups.get (groupName) != null) || (_privatePeerGroups.get (groupName) != null)) {
                byte[] encryptedParam = MessageCodec.getEncryptedParamFromPeerSearchReplyMessage (message);
                if (encryptedParam != null) {
                    byte[] param = null;
                    try {
                        param = CryptoUtils.decryptUsingPrivateKey (_keyPair.getPrivate(), encryptedParam);
                    }
                    catch (InvalidKeyException ike) {
                        ike.printStackTrace();
                    }
                    if (param != null) {
                        _listener.peerSearchResultReceived (groupName, nodeUUID, searchUUID, param);
                    }
                }
            }
            else {
                _logger.warning ("received a peer search reply from a group I am not a member of: " + groupName);
            }
        }
    }

    /**
     * Handle a GROUP_DATA message (received via UDP)
     *
     * @param nodeUUID       the group nodeUUID
     * @param message        the message byte array
     * @param messageLen     the message length
     */
    private synchronized void handlePeerGroupDataMessage (String nodeUUID, byte[] message, int messageLen)
    {
        PeerInfo pi = (PeerInfo) _peers.get (nodeUUID);
        if (pi != null) {
            int stateSeqNo = MessageCodec.getStateSeqNoFromGroupDataMessage (message);
            if ((pi.grpDataStateSeqNo != -1) && (stateSeqNo < pi.grpDataStateSeqNo) && (stateSeqNo > (pi.grpDataStateSeqNo - MAX_SEQUENCE_NUM/2))) {
                // the state did not change - we don't need to process this message
                return;
            }
            String groupName =  MessageCodec.getGroupNameFromGroupDataMessage (message);
            RemoteGroupInfo rgi = (RemoteGroupInfo) pi.groups.get (groupName);
            if (rgi == null) {
                // The group does not exist
                _logger.warning ("group " + groupName + " does not exist in the group list of node: " + nodeUUID);
                return;
            }

            if (rgi instanceof RemotePublicPeerGroupInfo) {
                // Update the data in the remote group info
                RemotePublicPeerGroupInfo rppgi = (RemotePublicPeerGroupInfo) rgi;
                byte[] groupData = MessageCodec.getDataFromGroupDataMessage (message);
                rppgi.data = (groupData != null) ? (byte[]) groupData.clone() : null;

                // Check to see if there is a local peer group with the same name, implying that the
                // remote peer group is a member of the local peer group
                LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) _publicPeerGroups.get (groupName);
                if (ppgi != null) {
                    // Notify the listener (if any) that the data has changed
                    if (_listener != null) {
                        _listener.peerGroupDataChanged (groupName, nodeUUID, rppgi.data);
                    }
                }
            }
            else if (rgi instanceof RemotePrivatePeerGroupInfo) {
                RemotePrivatePeerGroupInfo rppgi = (RemotePrivatePeerGroupInfo) rgi;
                byte[] encryptedGroupData = MessageCodec.getDataFromGroupDataMessage (message);
                rppgi.encryptedData = (encryptedGroupData != null) ? (byte[]) encryptedGroupData.clone() : null;
                rppgi.unencryptedDataLength = MessageCodec.getDataLenFromGroupDataMessage (message);
                // Make sure that the remote peer group is a member of the peer group locally
                // If not, there must have been a conflict with the password earlier
                LocalPrivatePeerGroupInfo rpgi = (LocalPrivatePeerGroupInfo) _privatePeerGroups.get (groupName);
                if (rpgi != null) {
                    // There is a local group with the same name - see if it has the peer node as a member
                    if (rpgi.members.get (nodeUUID) != null) {
                        // The peer node is a member of the local private peer group, which means that the
                        // passwords must match
                        /*!!*/ // Need to implement the password check to make sure the remote private peer group and
                               // the local private peer group are using the same password
                        if (_listener != null) {
                            try {
                                if (rppgi.encryptedData != null) {
                                    byte[] groupData = CryptoUtils.decryptUsingSecretKey (rpgi.key, rppgi.encryptedData);
                                    byte[] croppedGroupData = new byte[rppgi.unencryptedDataLength];
                                    System.arraycopy (groupData, 0, croppedGroupData, 0, rppgi.unencryptedDataLength);
                                    _listener.peerGroupDataChanged (groupName, nodeUUID, croppedGroupData);
                                }
                                else {
                                    _listener.peerGroupDataChanged (groupName, nodeUUID, null);
                                }
                            }
                            catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                    }
                    else {
                        // The local peer group does not have the remote peer as a member, so there must have been a conflict
                    }
                }
            }
            else {
                // The GroupInfo for the specified group is not a peer group - error
            }
        }
        else {
            // The group data message is from a node that is not known - send info message
            sendInfoMessage();
        }
    }

    /**
     * Send the PING message
     */
    private void sendPingMessage()
    {
        try {
            _nal.broadcastMessage (null, 0, MessageCodec.PING_MESSAGE, _pingHopCount, _pingFloodProb, false);
        }
        catch (NetworkException ne) {
            ne.printStackTrace();
        }
        _lastPingTime = System.currentTimeMillis();
    }

    /**
     * Send the INFO message
     */
    private void sendInfoMessage()
    {
        if (_advertiseNode && (_cachedInfoMessageLen > 0)) {
            try {
                _nal.broadcastMessage (_cachedInfoMessage, _cachedInfoMessageLen, MessageCodec.INFO_MESSAGE,
                                       _pingHopCount, _pingFloodProb, true);
            }
            catch (NetworkException ne) {
                ne.printStackTrace();
            }
            _lastInfoBCastTime = _lastPingTime = System.currentTimeMillis();
        }
    }

    /**
     * Update the INFO message
     */
    private synchronized void updateInfoMessage()
    {
        int hopCount = _pingHopCount;
        int floodProb = _pingFloodProb;

        // Compute the Hop Count and the Flood Probability finding the maximum value
        for (Enumeration e = _publicPeerGroups.elements(); e.hasMoreElements();) {
            LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) e.nextElement();
            if (ppgi.hopCount > hopCount) {
                hopCount = ppgi.hopCount;
            }
            if (ppgi.floodProbability > floodProb) {
                floodProb = ppgi.floodProbability;
            }
        }
        for (Enumeration e = _privatePeerGroups.elements(); e.hasMoreElements();) {
            LocalPrivatePeerGroupInfo rpgi = (LocalPrivatePeerGroupInfo) e.nextElement();
            if (rpgi.hopCount > hopCount) {
                hopCount = rpgi.hopCount;
            }
            if (rpgi.floodProbability > floodProb) {
                floodProb = rpgi.floodProbability;
            }
        }

        _infoStateSeqNo = ((_infoStateSeqNo+1) < MAX_SEQUENCE_NUM) ? (_infoStateSeqNo+1) : 0;
        _cachedInfoMessageLen = MessageCodec.createInfoMessage (_cachedInfoMessage, _infoStateSeqNo, _pingInterval,
                                                                _nodeName, _keyPair.getPublic(), _publicManagedGroups,
                                                                _privateManagedGroups, _publicPeerGroups,
                                                                _privatePeerGroups);
    }

    /**
     * Send a GROUP_DATA message
     *
     * @param groupName
     * @param unencryptedDataLength    unencrypted data length (data in encrypted only for private groups)
     * @param data
     * @param hopCount
     * @param floodProb
     * @param stateSeqNo
     */
    private synchronized void sendPeerGroupDataMessage (String groupName, int unencryptedDataLen, byte[] data, 
                                                        int hopCount, int floodProb, int stateSeqNo)
    {
        int messageLen = MessageCodec.createPeerGroupDataMessage (_groupDataMessage, stateSeqNo, groupName, 
                                                                  unencryptedDataLen, data);
        if (messageLen > 0) {
            try {
                _nal.broadcastMessage (_groupDataMessage, messageLen, MessageCodec.GROUP_DATA_MESSAGE,
                                       _pingHopCount, _pingFloodProb, false);
            }
            catch (NetworkException ne) {
                ne.printStackTrace();
            }
            _lastPingTime = System.currentTimeMillis();   // A GROUP_DATA message counts as a PING_MESSAGE also
        }
    }

    /**
     * Forward a Persistent Peer Search message to another node (if the node is
     * member of the searched group)
     *
     * @param nodeUUID    the node UUID
     */
    private synchronized void forwardPersistentPeerSearchMessages (String nodeUUID)
    {
        boolean forward = false;

        // Forward Persistent Peer Search message if the node is a member of the peer search group
        for (Enumeration e = _peerSearchInfos.elements(); e.hasMoreElements();) {
            PeerSearchInfo psi  = (PeerSearchInfo) e.nextElement();
            PeerInfo pi = (PeerInfo) _peers.get (nodeUUID);

            for (Enumeration en = pi.groups.elements(); en.hasMoreElements();) {
                RemoteGroupInfo rgi = (RemoteGroupInfo) en.nextElement();
                if ((rgi instanceof RemotePublicPeerGroupInfo) || (rgi instanceof RemotePrivatePeerGroupInfo)) {
                    if (rgi.groupName == psi.groupName) {
                        forward = true;
                        break;
                    }
                }
            }
            if (forward && (psi.nodeUUID != nodeUUID) && (psi.ttl != 0)) {
                if (psi.ttl > 0) {
                    // Update the TTL
                    if (psi.getUpdatedTTL() <= 0) {
                        // The TTL elapsed so I can remove the Persistent Peer Search
                        _peerSearchInfos.remove (psi.searchUUID);
                        continue;
                    }
                    MessageCodec.updateTTLInPeerSearchMessage (psi.message, psi.ttl);
                }

                // Unicast the (Persistent) Peer Search Message
                int messageType = psi.bPrivate ? MessageCodec.PEER_SEARCH_PPG_MESSAGE
                                                 : MessageCodec.PEER_SEARCH_MESSAGE;

                try {
                    _nal.sendMessage (psi.message, psi.messageLen, pi.nicsInfo, messageType, 0, true);
                }
                catch (NetworkException ne) {
                    ne.printStackTrace();
                }
            }
        }
    }

    /**
     * Timeout dead peers
     */
    private void timeoutNodes()
    {
        long currentTime = System.currentTimeMillis();
        boolean done = false;
        while (!done) {
            done = true;
            for (Enumeration e = _peers.elements(); e.hasMoreElements();) {
                 PeerInfo pi = (PeerInfo) e.nextElement();
                 long elapsedTime = currentTime - pi.lastContactTime;
                 if (elapsedTime > (pi.pingInterval * _nodeTimeoutFactor)) {
                     for (Enumeration en = _publicManagedGroups.elements(); en.hasMoreElements();) {
                         LocalPublicManagedGroupInfo pgi = (LocalPublicManagedGroupInfo) en.nextElement();
                         if (pgi.members.remove (pi.nodeUUID) != null) {
                             if (_listener != null) {
                                 _listener.groupMemberLeft (pgi.groupName, pi.nodeUUID);
                             }
                         }
                     }
                     for (Enumeration en = _privateManagedGroups.elements(); en.hasMoreElements();) {
                         LocalPrivateManagedGroupInfo rgi = (LocalPrivateManagedGroupInfo) en.nextElement();
                         if (rgi.members.remove (pi.nodeUUID) != null) {
                             if (_listener != null) {
                                 _listener.groupMemberLeft (rgi.groupName, pi.nodeUUID);
                             }
                         }
                     }
                     for (Enumeration en = _publicPeerGroups.elements(); en.hasMoreElements();) {
                         LocalPublicPeerGroupInfo ppgi = (LocalPublicPeerGroupInfo) en.nextElement();
                         if (ppgi.members.remove (pi.nodeUUID) != null) {
                             if (_listener != null) {
                                 _listener.groupMemberLeft (ppgi.groupName, pi.nodeUUID);
                             }
                         }
                     }
                     for (Enumeration en = _privatePeerGroups.elements(); en.hasMoreElements();) {
                         LocalPrivatePeerGroupInfo rpgi = (LocalPrivatePeerGroupInfo) en.nextElement();
                         if (rpgi.members.remove (pi.nodeUUID) != null) {
                             if (_listener != null) {
                                 _listener.groupMemberLeft (rpgi.groupName, pi.nodeUUID);
                             }
                         }
                     }
                     for (Enumeration en = _joinedPublicManagedGroups.elements(); en.hasMoreElements();) {
                         PublicManagedGroupJoinInfo pmgji = (PublicManagedGroupJoinInfo) en.nextElement();
                         if (pmgji.creatorUUID.equals (pi.nodeUUID) && (pmgji.joinMode == JoinInfo.JOIN_ONE_TIME_ONLY)) {
		             _joinedPublicManagedGroups.remove (pmgji.groupName);
                             /*!!*/ // Do we need a callback to the application here???
                         }
                     }
                     for (Enumeration en = _joinedPrivateManagedGroups.elements(); en.hasMoreElements();) {
                         PrivateManagedGroupJoinInfo pmgji = (PrivateManagedGroupJoinInfo) en.nextElement();
                         if (pmgji.creatorUUID.equals (pi.nodeUUID) && (pmgji.joinMode == JoinInfo.JOIN_ONE_TIME_ONLY)) {
                             _joinedPrivateManagedGroups.remove (pmgji.groupName);
                             /*!!*/ // Do we need a callback to the application here???
                         }
                     }

                     compileGroupList();
                     if (_listener != null) {
                        _listener.deadPeer (pi.nodeUUID);
                     }

                     // removing the PeerInfo after notifying the listener so that it can gather information about the 
                     // peer in the groupMemberLeft() and deadPeer() callbacks.
                     _peers.remove (pi.nodeUUID);
                     _deadPeers.put (pi.nodeUUID, pi);

                     done = false;    // Since the hashtable has been changed, start iterating over the hashtable again
                     break;
                 }
            }
        }
    }

    /**
     * Delete expired persistent peer searches
     */
    private void timeoutPersistentPeerSearches()
    {
        // Resend Persistent Peer Search messages and remove timed out peer search infos
        for (Enumeration en = _peerSearchInfos.elements(); en.hasMoreElements();) {
            PeerSearchInfo psi = (PeerSearchInfo) en.nextElement();

            if ( ((!psi.nodeUUID.equals (getNodeUUID())) && (psi.getTimeSinceLastRx() > _maxPeerSearchLeaseTime) ||
                 ( (psi.ttl > 0) && (psi.getUpdatedTTL() <= 0))) ) {
                if (psi.ttl > 0) {
                    _logger.info ("removing persistent peer search: " + psi.searchUUID);
                }
                if (_listener != null) {
                    _listener.persistentPeerSearchTerminated (psi.groupName, psi.nodeUUID, psi.searchUUID);
                }
                _peerSearchInfos.remove (psi.searchUUID);
            }
            else if (psi.nodeUUID.equals (getNodeUUID()) && psi.ttl != 0) {
                // if I am the originator of the persistent peer search I need to check
                // whether I have to rebroadcast the message
                if (psi.getTimeSinceLastTx() > getPeerSearchResendInterval()) {
                    psi.lastTxTime = System.currentTimeMillis();

                    // Update the TTL
                    if (psi.ttl > 0) {
                        MessageCodec.updateTTLInPeerSearchMessage (psi.message, psi.ttl);
                    }

                    _logger.fine ("resending Persistent Peer Search: " + psi.searchUUID + " - " +
                                  " group name: " + psi.groupName + ", (TTL: " +
                                  psi.ttl + ")");


                    int messageType = (psi.bPrivate) ? MessageCodec.PEER_SEARCH_PPG_MESSAGE
                                                       : MessageCodec.PEER_SEARCH_MESSAGE;
                    try {
                        _nal.broadcastMessage (psi.message, psi.messageLen, messageType,
                                               psi.hopCount, psi.floodProb, true);
                    }
                    catch (NetworkException ne) {
                        ne.printStackTrace();
                    }
                }
            }
        }
    }

    /**
     *  Add local and remote groups to the groups hashtable
     */
    private void compileGroupList()
    {
        Hashtable oldGroups = _groups;
        _groups = new Hashtable();
        // Add locally defined groups
        Enumeration e = _publicManagedGroups.elements();
        while (e.hasMoreElements()) {
            LocalPublicManagedGroupInfo gi = (LocalPublicManagedGroupInfo) e.nextElement();
            _groups.put (gi.groupName, gi);
        }
        e = _privateManagedGroups.elements();
        while (e.hasMoreElements()) {
            LocalPrivateManagedGroupInfo rgi = (LocalPrivateManagedGroupInfo) e.nextElement();
            _groups.put (rgi.groupName, rgi);
        }
        e = _publicPeerGroups.elements();
        while (e.hasMoreElements()) {
            LocalPublicPeerGroupInfo pgi = (LocalPublicPeerGroupInfo) e.nextElement();
            _groups.put (pgi.groupName, pgi);
        }
        e = _privatePeerGroups.elements();
        while (e.hasMoreElements()) {
            LocalPrivatePeerGroupInfo rpgi = (LocalPrivatePeerGroupInfo) e.nextElement();
            _groups.put (rpgi.groupName, rpgi);
        }
        // Add groups defined by remote group managers
        e = _peers.elements();
        while (e.hasMoreElements()) {
            PeerInfo pi = (PeerInfo) e.nextElement();
            Enumeration eg = pi.groups.elements();
            while (eg.hasMoreElements()) {
                RemoteGroupInfo rgi = (RemoteGroupInfo) eg.nextElement();
                if (rgi instanceof RemotePublicPeerGroupInfo) {
                    RemotePublicPeerGroupInfo rppgi = (RemotePublicPeerGroupInfo) rgi;
                    GroupInfo gi = (GroupInfo) _groups.get (rppgi.groupName);
                    if (gi == null) {
                        // The local node does not have this group defined
                        _groups.put (rppgi.groupName, rppgi);
                    }
                }
                else if (rgi instanceof RemotePrivatePeerGroupInfo) {
                    RemotePrivatePeerGroupInfo rppgi = (RemotePrivatePeerGroupInfo) rgi;
                    GroupInfo gi = (GroupInfo) _groups.get (rppgi.groupName);
                    if (gi == null) {
                        _groups.put (rppgi.groupName, rppgi);
                    }
                }
                else {
                    _groups.put (rgi.groupName + "@" + rgi.creatorUUID, rgi);
                    // Check to see if this group was already present
                    GroupInfo giOld = (GroupInfo) oldGroups.get (rgi.groupName + "@" + rgi.creatorUUID);
                    if ((giOld != null) && (giOld instanceof RemoteGroupInfo)) {
                        // Check if this local group manager had already joined the remote group
                        if (((RemoteGroupInfo)giOld).joined) {
                            rgi.joined = true;
                        }
                    }
                }
            }
        }
    }

    /**
     *  Join a Public Managed Group
     *
     * @param ji      public managed group JoinInfo
     */
    private void joinPublicManagedGroupAtPeer (PublicManagedGroupJoinInfo ji)
        throws GroupManagerException
    {
        try {
            PeerInfo pi = (PeerInfo) _peers.get (ji.creatorUUID);
            if (pi == null) {
                throw new GroupManagerException ("unknown peer " + ji.creatorUUID);
            }
            RemoteGroupInfo rgi = (RemoteGroupInfo) pi.groups.get (ji.groupName);
            if (rgi == null) {
                throw new GroupManagerException ("group <" + ji.groupName + "> is unknown at peer " + ji.creatorUUID);
            }
            if (!(rgi instanceof RemotePublicManagedGroupInfo)) {
                throw new GroupManagerException ("group <" + ji.groupName + "> is not a public group");
            }

            Socket s = new Socket (pi.addr, pi.port);
            CommHelper ch = new CommHelper();
            ch.init (s);
            CommHelper chOutput = new CommHelper();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();

            boolean useEncryption = false;
            if (pi.pubKey != null) {
                useEncryption = true;
            }

            if (useEncryption) {
                SecureOutputStream sos = CryptoUtils.encryptUsingPublicKey (pi.pubKey, baos);
                chOutput.init (null, sos);
            }
            else {
                chOutput.init (null, baos);    
            }
            
            chOutput.sendLine ("JOIN PublicManagedGroup");
            chOutput.sendLine ("UUID " + _nodeUUID);
            chOutput.sendLine ("GroupName " + ji.groupName);
            chOutput.sendLine ("JoinDataLength");
            byte[] joinData = ji.joinData;
            if (joinData != null) {
                chOutput.sendBlock (ByteArray.intToByteArray (joinData.length));
                chOutput.sendLine ("JoinData");
                chOutput.sendBlock (joinData);
            }
            else {
                chOutput.sendBlock (ByteArray.intToByteArray (0));
            }
            chOutput.sendLine ("PublicKey");
            chOutput.sendBlock (_keyPair.getPublic().getEncoded());
            byte[] message = baos.toByteArray();
            if (useEncryption) {
                ch.sendLine ("EncryptedBlock");
            }
            else {
                ch.sendLine ("UnencryptedBlock");
            }
            ch.sendBlock (message);
            ch.receiveMatch ("OK");
            ch.sendLine ("GoodBye");
            s.close();
            rgi.joined = true;
        }
        catch (Exception e) {
            //e.printStackTrace();
            throw new GroupManagerException ("failed to join group - nested exception <" + e + ">");
        }
    }

    /**
     *  Join a Private Managed Group
     *
     * @param ji      private managed group JoinInfo
     */
    private void joinPrivateManagedGroupAtPeer (PrivateManagedGroupJoinInfo ji)
        throws GroupManagerException
    {
        try {
            PeerInfo pi = (PeerInfo) _peers.get (ji.creatorUUID);
            if (pi == null) {
                throw new GroupManagerException ("unknown peer " + ji.creatorUUID);
            }
            RemoteGroupInfo rgi = (RemoteGroupInfo) pi.groups.get (ji.groupName);
            if (rgi == null) {
                throw new GroupManagerException ("group <" + ji.groupName + "> is unknown at peer " + ji.creatorUUID);
            }
            if (!(rgi instanceof RemotePrivateManagedGroupInfo)) {
                throw new GroupManagerException ("group <" + ji.groupName + "> is not a private group");
            }
            Socket s = new Socket (pi.addr, pi.port);
            CommHelper ch = new CommHelper();
            ch.init (s);
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            CommHelper chOutput = new CommHelper();

            boolean useEncryption = false;
            if (pi.pubKey != null) {
                useEncryption = true;
            }

            if (useEncryption) {
                SecureOutputStream sos = CryptoUtils.encryptUsingPublicKey (pi.pubKey, baos);
                chOutput.init (null, sos);
            }
            else {
                chOutput.init (null, baos);
            }
            
            chOutput.sendLine ("JOIN PrivateManagedGroup");
            chOutput.sendLine ("UUID " + _nodeUUID);
            chOutput.sendLine ("GroupName " + ji.groupName);
            chOutput.sendLine ("JoinDataLength");
            byte[] joinData = ji.joinData;
            if (joinData != null) {
                chOutput.sendBlock (ByteArray.intToByteArray (joinData.length));
                chOutput.sendLine ("JoinData");
                chOutput.sendBlock (joinData);
            }
            else {
                chOutput.sendBlock (ByteArray.intToByteArray (0));
            }
            chOutput.sendLine ("PublicKey");
            chOutput.sendBlock (_keyPair.getPublic().getEncoded());
            byte[] encryptedNonce = Base64Transcoders.convertB64StringToByteArray (
                    ((RemotePrivateManagedGroupInfo)rgi).encryptedNonce);
            String nonce = CryptoUtils.decryptStringUsingSecretKey (
                    CryptoUtils.generateSecretKey(ji.password), encryptedNonce);
            chOutput.sendLine ("Nonce " + nonce);
            byte[] message = baos.toByteArray();
            if (useEncryption) {
                ch.sendLine ("EncryptedBlock");
            }
            else {
                ch.sendLine ("UnencryptedBlock");
            }
            ch.sendBlock (message);
            ch.receiveMatch ("OK");
            ch.sendLine ("GoodBye");
            s.close();
            rgi.joined = true;
        }
        catch (Exception e) {
            e.printStackTrace();
            throw new GroupManagerException ("failed to join group - nested exception <" + e + ">");
        }
    }

    /**
     *  Leave a Manager Group
     *
     *  @param ji      the join info object that represents joining a group
     */
    private void leaveGroupAtPeer (JoinInfo ji)
        throws GroupManagerException
    {
        try {
            PeerInfo pi = (PeerInfo) _peers.get (ji.creatorUUID);
            if (pi == null) {
                throw new GroupManagerException ("unknown peer " + ji.creatorUUID);
            }
            RemoteGroupInfo rgi = (RemoteGroupInfo) pi.groups.get (ji.groupName);
            if (rgi == null) {
                throw new GroupManagerException ("group <" + ji.groupName + "> is unknown at peer " + ji.creatorUUID);
            }

            Socket s = new Socket (pi.addr, pi.port);
            CommHelper ch = new CommHelper();
            ch.init (s);
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            SecureOutputStream sos = CryptoUtils.encryptUsingPublicKey (pi.pubKey, baos);
            CommHelper chOutput = new CommHelper();
            chOutput.init (null, sos);
            chOutput.sendLine ("LEAVE Group");
            chOutput.sendLine ("UUID " + _nodeUUID);
            chOutput.sendLine ("GroupName " + ji.groupName);
            byte[] encryptedMessage = baos.toByteArray();
            ch.sendLine ("EncryptedBlock");
            ch.sendBlock (encryptedMessage);
            ch.receiveMatch ("OK");
            ch.sendLine ("GoodBye");
            s.close();
            rgi.joined = false;
        }
        catch (Exception e) {
            throw new GroupManagerException ("failed to leave group - nested exception <" + e + ">");
        }
    }

    /**
     * Add a Remote Public Peer Group as a member
     *
     * @param ppgi
     * @param rppgi
     * @return
     */
    private boolean addRemotePublicPeerGroup (LocalPublicPeerGroupInfo ppgi, RemotePublicPeerGroupInfo rppgi)
    {
        // Get the peer group manager
        PeerInfo pi = (PeerInfo) _peers.get (rppgi.creatorUUID);
        if (pi == null) {
            return false;
        }
        PublicPeerGroupMemberInfo ppgmi = new PublicPeerGroupMemberInfo();
        ppgmi.nodeUUID = pi.nodeUUID;
        ppgmi.addr = pi.addr;
        ppgmi.port = pi.port;
        ppgmi.pubKey = pi.pubKey;
        ppgmi.data = (rppgi.data != null) ? (byte[]) rppgi.data.clone() : null;
        ppgi.members.put (ppgmi.nodeUUID, ppgmi);
        if (_listener != null) {
            if (rppgi.data == null) {
                _listener.newGroupMember (ppgi.groupName, ppgmi.nodeUUID, null);
            }
            else {
                _listener.newGroupMember (ppgi.groupName, ppgmi.nodeUUID, rppgi.data);
            }
        }
        return true;
    }

    /**
     * Add a Remote Private Peer Group as a member
     *
     * @param rpgi
     * @param rppgi
     * @return
     */
    private boolean addRemotePrivatePeerGroup (LocalPrivatePeerGroupInfo rpgi, RemotePrivatePeerGroupInfo rppgi)
    {
        // Ensure that the same password has been used for the group
        if (!rpgi.encryptedGroupName.equals (rppgi.encryptedGroupName)) {
            if (_listener != null) {
                _listener.conflictWithPrivatePeerGroup (rpgi.groupName, rppgi.creatorUUID);
            }
            return false;
        }
        // Decode the peer's encrypted nonce
        byte[] decryptedData = null;
        byte[] choppedDecryptedData = null;
        String decryptedNonce = null;
        try {
            byte[] encryptedNonce = Base64Transcoders.convertB64StringToByteArray (rppgi.encryptedNonce);
            decryptedNonce = CryptoUtils.decryptStringUsingSecretKey (
                    CryptoUtils.generateSecretKey (rpgi.password), encryptedNonce);
            if (rppgi.encryptedData != null) {
                decryptedData = CryptoUtils.decryptUsingSecretKey (
                    CryptoUtils.generateSecretKey (rpgi.password), rppgi.encryptedData);
                choppedDecryptedData = new byte[rppgi.unencryptedDataLength];
                System.arraycopy (decryptedData, 0, choppedDecryptedData, 0, rppgi.unencryptedDataLength);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        // Get the peer group manager info
        PeerInfo pi = (PeerInfo) _peers.get (rppgi.creatorUUID);
        if (pi == null) {
            return false;
        }
        PrivatePeerGroupMemberInfo ppgmi = new PrivatePeerGroupMemberInfo();
        ppgmi.nodeUUID = pi.nodeUUID;
        ppgmi.addr = pi.addr;
        ppgmi.port = pi.port;
        ppgmi.pubKey = pi.pubKey;
        ppgmi.decryptedNonce = decryptedNonce;
        ppgmi.data = (choppedDecryptedData != null) ? (byte[]) choppedDecryptedData.clone() : null;
        rpgi.members.put (ppgmi.nodeUUID, ppgmi);
        if (_listener != null) {
            if (rppgi.encryptedData != null) {
                try {
                    byte[] data = CryptoUtils.decryptUsingSecretKey (rpgi.key, rppgi.encryptedData);
                    byte[] choppedData = new byte[rppgi.unencryptedDataLength];
                    System.arraycopy (data, 0, choppedData, 0, rppgi.unencryptedDataLength);
                    if (data != null) {
                        //_listener.peerGroupDataChanged (rpgi.groupName, ppgmi.nodeUUID, data);
                        _listener.newGroupMember (rpgi.groupName, ppgmi.nodeUUID, choppedData);
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
            else {
                _listener.newGroupMember (rpgi.groupName, ppgmi.nodeUUID, null);
            }
        }
        return true;
    }

    /**
     * Validate a group name. 
     * <p>
     * Group names have to be unique and cannot contain spaces.
     *
     * @param groupName
     * @return <code>true</code> if <code>groupName</code>
     *         is a valid group name.
     */
    private boolean validateGroupName (String groupName)
    {
        if (groupName == null) {
            _logger.warning ("group name is null");
            return false;
        }

        if (_publicPeerGroups.get (groupName) != null ||
            _privatePeerGroups.get (groupName) != null ||
            _publicManagedGroups.get (groupName) != null ||
            _privateManagedGroups.get (groupName) != null) {
            _logger.warning ("a group with the same name already exists");
            return false;
        }

        if (groupName.indexOf (" ") != -1) {
            _logger.warning ("group names with spaces are not allowed");
            return false;
        }

        return true;
    }

    /**
     * Validate a password
     * <p>
     * Currently we are simply checking if the password is not null.
     * In the future we might add additinal checks, such as requirements
     * on the password lenght.
     *
     * @param password
     * @return <code>true</code> if the password is valid
     */ 
    private boolean validatePassword (String password)
    {
        if (password == null) {
            return false;    
        }
        return true;
    }

    //Class Variables
    public static final int DEFAULT_PORT = 8500;
    public static final int DEFAULT_PING_HOP_COUNT = 1;
    public static final int DEFAULT_PING_INTERVAL = 2000;
    public static final int DEFAULT_PING_FLOOD_PROB = 100;
    public static final int DEFAULT_NODE_TIMEOUT_FACTOR = 5;

    public static final int DEFAULT_INFO_BCAST_INTERVAL = 10000;

    public static final int DEFAULT_PEER_SEARCH_HOP_COUNT = 3;
    public static final int DEFAULT_PEER_SEARCH_FLOOD_PROB = 100;
    public static final int DEFAULT_PEER_SEARCH_TTL = 0;
    public static final int DEFAULT_PEER_SEARCH_CACHE_TIME = 1000;

    public static final int PEER_SEARCH_LEASE_TIME = 20000; // NOTE: DEFAULT_MAX_ALLOWED_PEER_SEARCH_TTL > DEFAULT_PEER_SEARCH_RESEND_INTERVAL
    public static final int DEFAULT_PEER_SEARCH_RESEND_INTERVAL = 10000;

    public static final int MAX_GROUP_NAME_LEN = 255;
    public static final int MAX_JOIN_PARAM_SIZE = 2048;
    public static final int MAX_UUID_LEN = 127;

    private int _port = 0;
    private String _nodeUUID;
    private String _nodeName;
    private int _infoStateSeqNo = 0;
    private int _groupDataStateSeqNo = 0;
    private int _peerSearchFloodProb = DEFAULT_PEER_SEARCH_FLOOD_PROB;
    private int _peerSearchHopCount = DEFAULT_PEER_SEARCH_HOP_COUNT;
    private int _peerSearchTTL = DEFAULT_PEER_SEARCH_TTL;
    private int _peerSearchResendInterval = DEFAULT_PEER_SEARCH_RESEND_INTERVAL;
    private int _maxPeerSearchLeaseTime = PEER_SEARCH_LEASE_TIME;
    private int _pingFloodProb = DEFAULT_PING_FLOOD_PROB;
    private int _pingHopCount = DEFAULT_PING_HOP_COUNT;
    private int _pingInterval = DEFAULT_PING_INTERVAL;
    private byte _nodeTimeoutFactor = DEFAULT_NODE_TIMEOUT_FACTOR;
    private int _infoBCastInterval = DEFAULT_INFO_BCAST_INTERVAL;
    private long _lastInfoBCastTime = 0;
    private long _lastPingTime = 0;
    private KeyPair _keyPair;

    private Hashtable _publicManagedGroups = new Hashtable();   	// Hashtable<String, LocalPublicManagedGroupInfo> - keys are group names
    private Hashtable _privateManagedGroups = new Hashtable();  	// Hashtable<String, LocalPrivateManagedGroupInfo> - keys are group names
    private Hashtable _publicPeerGroups = new Hashtable();      	// Hashtable<String, LocalPublicPeerGroupInfo> - keys are group names
    private Hashtable _privatePeerGroups = new Hashtable();     	// Hashtable<String, LocalPrivatePeerGroupInfo> - keys are group names
    private Hashtable _joinedPublicManagedGroups = new Hashtable(); // Hashtable<String, PublicManagedGroupJoinInfo> - keys are group names
    private Hashtable _joinedPrivateManagedGroups = new Hashtable();// Hashtable<String, PrivateManagedGroupJoinInfo> - keys are group names

    private Hashtable _peerSearchInfos = new Hashtable();       	// Hashtable<String, PeerSearchInfo> - keys are searchUUIDs

    private byte[] _cachedInfoMessage = new byte [MessageCodec.MAX_MESSAGE_SIZE];
    private byte[] _groupDataMessage = new byte [MessageCodec.MAX_MESSAGE_SIZE];
    private int _cachedInfoMessageLen = 0;

    private GroupManagerListener _listener;

    private boolean _running = false;
    private boolean _terminate = false;
    private boolean _terminated = false;
    private boolean _advertiseNode = true;
    private Exception _terminatingException;

    private Hashtable _peers;       // Hashtable<String, PeerInfo>  - keys are nodeUUIDs
    private Hashtable _deadPeers;   // Hashtable<String, PeerInfo>  - keys are nodeUUIDs
    private Hashtable _groups;      // Hashtable<String, GroupInfo> - keys are group names
    
    private NetworkAccessLayer _nal;
    private TCPServer _tcpServer;
    private Logger _logger;
    private static final int MAX_SEQUENCE_NUM = 65535; // 2^16
}
