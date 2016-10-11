package us.ihmc.aci.disServiceProProxy;

import java.io.ByteArrayOutputStream;
import java.util.*;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxy;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxyListener;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.StringUtil;

public class DisServiceProProxy extends DisseminationServiceProxy implements DisServiceProProxyInterface, MatchmakingLogListener
{
    public DisServiceProProxy ()
    {
        super (new us.ihmc.aci.disServiceProProxy.CallbackHandlerFactory());
        _matchmakerListeners = new ArrayList<MatchmakingLogListener>();
    }

    public DisServiceProProxy (short applicationId)
    {
        super (new us.ihmc.aci.disServiceProProxy.CallbackHandlerFactory(), applicationId);
        _matchmakerListeners = new ArrayList<MatchmakingLogListener>();
    }

    public DisServiceProProxy (short applicationId, long reinitializationAttemptInterval)
    {
        super (new us.ihmc.aci.disServiceProProxy.CallbackHandlerFactory(), applicationId, reinitializationAttemptInterval);
        _matchmakerListeners = new ArrayList<MatchmakingLogListener>();
    }

    /**
     * 
     * @param applicationId
     * @param host - the IP of the disservicepro proxy server in dot-decimal
     *               notation
     * @param iPort 
     */
    public DisServiceProProxy (short applicationId, String host, int iPort)
    {
    	super (new us.ihmc.aci.disServiceProProxy.CallbackHandlerFactory(), applicationId, host, iPort);
        _matchmakerListeners = new ArrayList<MatchmakingLogListener>();
    }

    public DisServiceProProxy (short applicationId, String host, int port, long reinitializationAttemptInterval)
    {
        super (new us.ihmc.aci.disServiceProProxy.CallbackHandlerFactory(), applicationId, host, port, reinitializationAttemptInterval);
        _matchmakerListeners = new ArrayList<MatchmakingLogListener>();
    }

    public synchronized void registerDisServiceProProxyListener (DisServiceProProxyListener listener)
        throws CommException
    {
        checkConcurrentModification ("registerDisServiceProProxyListener");

        if (_listeners == null) {
            _LOGGER.severe ("Error:DisServiceProProxyListener is null");
            return;
        }
        if (_listeners.isEmpty()) {
            _listeners.put((short)0, listener);
            try {
                _commHelper.sendLine ("registerPathRegisteredCallback");
            }
            catch (Exception e) {
                _LOGGER.severe (StringUtil.getStackTraceAsString (e));
                throw new CommException (e);
            }
        }
         else {
            // Search for an unused ID
            short i = 0;
            for (; _listeners.containsKey(i); i++);
            _listeners.put(i, listener);
        }
    }

    public synchronized void registerMatchmakingLogListener (MatchmakingLogListener listener)
        throws CommException
    {
        checkConcurrentModification("registerMatchmakingLogListener");

        if (listener == null) {
            _LOGGER.severe ("Error:MatchmakingLogListener is null");
            return;
        }
        if (_matchmakerListeners.isEmpty()) {
            _matchmakerListeners.add (listener);
            try {
                _commHelper.sendLine ("registerMatchmakingLogCallback");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                _LOGGER.log(Level.SEVERE, null, e);
            }
        }
        else {
            _matchmakerListeners.add(listener);
        }
    }

    public synchronized int configureMetaDataFields (String xMLMetadataFields)
    {
    	// TODO: implement this
    	return 0;
    }

    public synchronized int configureMetaDataFields (String[] attributes, String[] values)
    {
    	// TODO: implement this
    	return 0;
    }

    public synchronized boolean configureProperties (Properties properties) throws CommException
    {
        checkConcurrentModification("configureProperties");

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        try {
            _commHelper.sendLine ("configureProperties");
            properties.store (out, null);
            _commHelper.write32 (out.size());
            _commHelper.sendBlob (out.toByteArray());

            _commHelper.receiveMatch ("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
        }

    	return false;
    }

    public synchronized boolean setMetadataPossibleValues (String xMLMetadataValues) throws CommException
    {
        checkConcurrentModification("setMetadataPossibleValues");

    	try {
            _commHelper.sendLine ("setMetadataPossibleValues");
            _commHelper.write32 (xMLMetadataValues.length());
            _commHelper.sendBlob (xMLMetadataValues.getBytes());

            _commHelper.receiveMatch ("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
    	}

    	return false;
    }

    public synchronized boolean setMetadataPossibleValues (String[] attributes, String[] values) throws CommException
    {
    	checkConcurrentModification("setMetadataPossibleValues");

    	try {

            // TODO: implement this

            return true;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return false;
    	}
    }

    public synchronized boolean setRankingWeights (float coordRankWeight, float timeRankWeight,
                                                   float expirationRankWeight, float impRankWeight,
                                                   float predRankWeight, float targetWeight,
                                                   boolean strictTarget) throws CommException
    {
        checkConcurrentModification("setRankingWeights");

    	try {

            // TODO: implement this

            return true;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return false;
    	}
    }

    public synchronized boolean registerPath (NodePath path) throws CommException
    {
        checkConcurrentModification("registerPath");

    	try {
            _commHelper.sendLine ("registerPath");

            if (!path.write (_commHelper)) {
                _LOGGER.severe ("DisseminationServiceProProxy-registerPath: Failed to send the path");
                return false;
            }

            _commHelper.receiveMatch ("OK");
            return true;
    	}
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return false;
    	}
    }

    public synchronized boolean setActualPath (String pathID) throws CommException
    {
        checkConcurrentModification("setActualPath");

    	try {
            _commHelper.sendLine ("setActualPath");
            _commHelper.sendLine (pathID);

            _commHelper.receiveMatch ("OK");
            return true;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return false;
    	}
    }

    public synchronized boolean setNodeContext (String teamID, String missionID, String role) throws CommException
    {
        checkConcurrentModification("setNodeContext");

    	try {
            _commHelper.sendLine ("setNodeContext");
            _commHelper.sendLine (teamID);
            _commHelper.sendLine (missionID);
            _commHelper.sendLine (role);

            _commHelper.receiveMatch ("OK");
            return true;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return false;
    	}
    }

    public synchronized int changePathProbability (String pathID, float fNewProbability)
    {
    	// TODO: implement this
    	return 0;
    }

    public synchronized int deregisterPath (String pathID)
    {
    	// TODO: implement this
    	return 0;
    }

    public synchronized NodePath getActualPath()
        throws CommException
    {
        checkConcurrentModification("getActualPath");

    	try {
            _commHelper.sendLine ("getActualPath");
            _commHelper.receiveMatch ("OK");

            NodePath newPath = NodePath.read (_commHelper);

            _commHelper.sendLine ("OK");
            return newPath;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    public synchronized NodePath getPathForPeer (String peerNodeId)
        throws CommException
    {
        if (peerNodeId == null || peerNodeId.length() == 0) {
            
        }
        checkConcurrentModification("getPathForPeer");

    	try {
            _commHelper.sendLine ("getPathForPeer");

            _commHelper.sendLine (peerNodeId);
            String match = _commHelper.receiveLine();
            NodePath newPath = null;
            if (match.equalsIgnoreCase ("NOPATH")) {
                return null;
            }
            else if (match.equalsIgnoreCase ("OK")) {
                newPath = NodePath.read (_commHelper);
            }
            else {
                throw new ProtocolException ("no match found: looked for OK or NOPATH in {" + match + "}");
            }

            _commHelper.sendLine ("OK");
            return newPath;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    public synchronized NodePath getPath()
    {
    	// TODO: implement this
    	return null;
    }

    public synchronized boolean setActualPosition (float fLatitude, float fLongitude, float fAltitude,
                                                   String location, String note) throws CommException
    {
        checkConcurrentModification("setActualPosition");

    	if (location == null) {
            location = "Default";
        }
        if (note == null) {
            note = "Default";
        }

    	try {
            _commHelper.sendLine ("setActualPosition");
            _commHelper.sendLine (Float.toString(fLatitude));
            _commHelper.sendLine (Float.toString(fLongitude));
            _commHelper.sendLine (Float.toString(fAltitude));

            _commHelper.sendBlock (location.getBytes());
            _commHelper.sendBlock (note.getBytes());

            _commHelper.receiveMatch ("OK");
            return true;
    	}
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return false;
    	}
    }

    public synchronized String getXMLMetaDataConfiguration()
    {
    	// TODO: implement this
    	return null;
    }

    public synchronized void getMetaDataConfiguration (String[] attributes, String[] values)
    {
    	// TODO: implement this
    }

    public synchronized List<String> getAllCachedMetaDataAsXML()
        throws CommException
    {
        return getAllCachedMetaDataAsXML (0, 0);
    }

    public synchronized List<String> getAllCachedMetaDataAsXML (long startTimestamp, long endTimestamp)
        throws CommException
    {
        return getMatchingMetaDataAsXML (new HashMap<String, String>(), 0, 0);
    }

    public synchronized List<String> getMatchingMetaDataAsXML (Map<String, String> attributeValueToMatch)
        throws CommException
    {
        return getMatchingMetaDataAsXML (attributeValueToMatch, 0, 0);
    }

    public synchronized List<String> getMatchingMetaDataAsXML (Map<String, String> attributeValueToMatch, long startTimestamp, long endTimestamp)
        throws CommException
    {
    	checkConcurrentModification("getMatchingMetaDataAsXML");

        List<String> metadata = new LinkedList<String>();
        try {
            _commHelper.sendLine ("getMatchingMetaDataAsXML");
            _commHelper.write32 (attributeValueToMatch.size());
            for (Entry<String, String> av : attributeValueToMatch.entrySet()) {
                _commHelper.sendStringBlock (av.getKey());
                _commHelper.sendStringBlock (av.getValue());
            }

            _commHelper.write64 (startTimestamp);
            _commHelper.write64 (endTimestamp);

            int metadataLen = _commHelper.read32();
            while (metadataLen > 0) {
                metadata.add (new String (_commHelper.receiveBlob (metadataLen)));
                metadataLen = _commHelper.read32();
            }

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}

        return metadata;
    }

    public synchronized PeerNodeContext getPeerNodeContext (String peerNodeId)
        throws CommException
    {
        checkConcurrentModification("getPeerNodeContext");

        try {
            _commHelper.sendLine ("getPeerNodeContext");
            _commHelper.sendBlock(peerNodeId.getBytes());
            String hasNodeContextMsg = _commHelper.receiveLine();
            PeerNodeContext peerNodeContext;
            if ("OK".equals(hasNodeContextMsg)) {
                peerNodeContext = PeerNodeContext.read(_commHelper);
            }
            else {
                _LOGGER.severe (hasNodeContextMsg);
                peerNodeContext = null;
            }
            return peerNodeContext;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    public synchronized byte[] getMetaDataAsXML (String messageID) throws CommException
    {
        checkConcurrentModification("getMetaDataAsXML");

    	int imetaDaLength;
    	byte[] metaData;
    	try {
            _commHelper.sendLine ("getMetaDataAsXML");
            _commHelper.sendLine (messageID);
            imetaDaLength = _commHelper.read32();
            metaData = _commHelper.receiveBlob (imetaDaLength);

            _commHelper.sendLine ("OK");
            return metaData;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    public synchronized byte[] getData (String messageID) throws CommException
    {
        checkConcurrentModification("getData");

    	int idataLength;
    	byte[] data;
    	try {
            _commHelper.sendLine ("getData");
            _commHelper.sendLine (messageID);
            idataLength = _commHelper.read32();
            if (idataLength > 0) {
                data = _commHelper.receiveBlob (idataLength);
            }
            else {
                data = null;
            }
            _commHelper.sendLine ("OK");
            return data;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    public synchronized String[] search (String groupName, String query) throws CommException
    {
        checkConcurrentModification("search");

    	short msgIdCount;
    	String[] msgIds;
    	try {
            _commHelper.sendLine ("search");
            _commHelper.sendLine (groupName);
            _commHelper.sendLine (query);

            // read the MsgId
            msgIdCount = _commHelper.read16();
            msgIds = new String[msgIdCount];
            for (int i = 0 ; i < msgIdCount ; i++) {
                msgIds[i] = _commHelper.receiveLine();
            }

            _commHelper.sendLine ("OK");
            return msgIds;
    	}
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    /**
     * 
     * @param iClientId - the unique id of the application connecting to DSProp
     * @param groupName - the name of the group into which the message will be
     *                    published
     * @param xMLMetadata - XML representation of the attributes that describe
     *                      the data
     * @param data - the actual data of the message that has to be published.
     * @param expirationTime - the length of time for which the published data is
     *                         considered relevant.  A value of 0 means that the
     *                         data is indefinitely relevant
     * @param historyWindow - the number of messages prior of the current one
     *                        that are necessary to correctly interpreter the
     *                         message being published
     * @param tag - tag is used to further specify the type on the message
     * @return the id assigned to the published message, null in case of error
     */
    public synchronized String pushPro (short iClientId, String groupName, String objectId, String instanceId,
                                        String xMLMetadata, byte[] data, long expirationTime, short historyWindow,
                                        short tag) throws CommException
    {
        checkConcurrentModification("pushPro - 0");

    	String msgId;
    	try {
            _commHelper.sendLine ("pushPro");
            _commHelper.write16 (iClientId);
            _commHelper.sendLine (groupName);
            _commHelper.sendStringBlock (objectId);
            _commHelper.sendStringBlock (instanceId);
            _commHelper.write32 (xMLMetadata.length());
            _commHelper.sendBlob (xMLMetadata.getBytes());
            _commHelper.write32 (data.length);
            _commHelper.sendBlob (data);
            _commHelper.write64 (expirationTime);
            _commHelper.write16 (historyWindow);
            _commHelper.write16 (tag);
            msgId = _commHelper.receiveLine();

            _commHelper.sendLine ("OK");
            return msgId;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    /**
     * @param iClientId - the unique id of the application connecting to DSProp
     * @param groupName - the name of the group into which the message will be
     *                    published
     * @param metaDataAttributes - the list of the attributes that describe the
     *                             data; corresponding attribute-value elements
     *                             must be stored at the same position in the
     *                             proper list (ex: if the 3rd element of
     *                             metaDataAttributes is LATITUDE, the corresponding
     *                             value, say -79.33 will be stored as 3rd element
     *                             in metaDataValues).
     * @param metaDataValues -the list of the value of the attributes that describe
     *                        the data; corresponding attribute-value elements
     *                        must be stored at the same position in the
     *                        proper list (ex: if the 3rd element of
     *                        metaDataAttributes is LATITUDE, the corresponding
     *                        value, say -79.33 will be stored as 3rd element in
     *                        metaDataValues).
     * @param data - the actual data of the message that has to be published.
     * @param expirationTime - the length of time for which the published data is
     *                         considered relevant.  A value of 0 means that the
     *                         data is indefinitely relevant
     * @param historyWindow - the number of messages prior of the current one
     *                        that are necessary to correctly interpreter the
     *                         message being published
     * @param tag - tag is used to further specify the type on the message
     * @return the id assigned to the published message, null in case of error
     */
    public synchronized String pushPro (short iClientId, String  groupName, String objectId, String instanceId,
                                        List<String> metaDataAttributes, List<String> metaDataValues,
                                        byte[] data, long expirationTime, short historyWindow, short tag) throws CommException
    {
        return pushPro (iClientId, groupName, objectId, instanceId,
                        metaDataAttributes.toArray(new String[metaDataAttributes.size()]),
                        metaDataValues.toArray(new String[metaDataValues.size()]),
                        data, expirationTime, historyWindow, tag);
    }

    /**
     * @param iClientId - the unique id of the application connecting to DSProp
     * @param groupName - the name of the group into which the message will be
     *                    published
     * @param metaDataAttributes - the array of the attributes that describe the
     *                             data; corresponding attribute-value elements
     *                             must be stored at the same position in the
     *                             proper array (ex: if the 3rd element of
     *                             metaDataAttributes is LATITUDE, the corresponding
     *                             value, say -79.33 will be stored as 3rd element
     *                             in metaDataValues).
     * @param metaDataValues -the array of the value of the attributes that describe
     *                        the data; corresponding attribute-value elements
     *                        must be stored at the same position in the proper
     *                        array (ex: if the 3rd element of metaDataAttributes
     *                        is LATITUDE, the corresponding value, say -79.33 will
     *                        be stored as 3rd element in metaDataValues).
     * @param data - the actual data of the message that has to be published.
     * @param expirationTime - the length of time for which the published data is
     *                         considered relevant.  A value of 0 means that the
     *                         data is indefinitely relevant
     * @param historyWindow - the number of messages prior of the current one
     *                        that are necessary to correctly interpreter the
     *                         message being published
     * @param tag - tag is used to further specify the type on the message
     * @return the id assigned to the published message, null in case of error
     */
    public synchronized String pushPro (short iClientId, String  groupName,
                                        String objectId, String instanceId,
                                        String[] metaDataAttributes, String[] metaDataValues,
                                        byte[] data, long expirationTime, short historyWindow,
                                        short tag) throws CommException
    {
        checkConcurrentModification("pushPro - 1");

    	String msgId;
    	try {
            _commHelper.sendLine ("pushPro_AVList");
            _commHelper.write16 (iClientId);
            _commHelper.sendLine (groupName);
            _commHelper.sendStringBlock (objectId);
            _commHelper.sendStringBlock (instanceId);

            _commHelper.write32(metaDataAttributes.length);
            for (int i = 0; i < metaDataAttributes.length; i++) {
                if (metaDataAttributes != null) {
                    _commHelper.write32(metaDataAttributes[i].length());
                    _commHelper.sendBlob(metaDataAttributes[i].getBytes());

                    if (metaDataValues[i] == null || metaDataValues[i].equals("")) {
                        // Sanity Check
                        // If no value is set, it has to be specified that the value
                        // is unknown
                        metaDataValues[i] = _UNKNOWN;
                    }
                    _commHelper.write32(metaDataValues[i].length());
                    _commHelper.sendBlob(metaDataValues[i].getBytes());
                }
            }

            _commHelper.write32 (data.length);
            _commHelper.sendBlob (data);
            _commHelper.write64 (expirationTime);
            _commHelper.write16 (historyWindow);
            _commHelper.write16 (tag);
            byte[] msgIdBuf = _commHelper.receiveBlock();
            if (msgIdBuf != null && msgIdBuf.length > 0) {
                msgId = new String (msgIdBuf);    
            }
            else {
                throw new Exception ("pushPro_AVList failed");
            }
            _commHelper.sendLine ("OK");
            return msgId;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    /**
     *
     * @param iClientId - the unique id of the application connecting to DSProp
     * @param groupName - the name of the group into which the message will be
     *                    published
     * @param xMLMetadata - XML representation of the attributes that describe
     *                      the data.
     * @param data - the actual data of the message that has to be published.
     * @param dataMimeType - the MIME type of the data being published.
     * @param expirationTime - the length of time for which the published data is
     *                         considered relevant.  A value of 0 means that the
     *                         data is indefinitely relevant.
     * @param historyWindow - the number of messages prior of the current one
     *                        that are necessary to correctly interpreter the
     *                         message being published.
     * @param tag - tag is used to further specify the type on the message.
     * @return the id assigned to the published message, null in case of error.
     */
    public synchronized String makeAvailablePro (short iClientId, String  groupName, String applicationId, String instanceId,
                                                 String xMLMetadata, byte[] data, String dataMimeType, long expirationTime,
                                                 short historyWindow, short tag)
        throws CommException
    {
        checkConcurrentModification("makeAvailablePro");

        String msgId;
    	try {
            _commHelper.sendLine ("makeAvailablePro");
            _commHelper.write16 (iClientId);
            _commHelper.sendLine (groupName);
            _commHelper.sendStringBlock (applicationId);
            _commHelper.sendStringBlock (instanceId);
            _commHelper.write32 (xMLMetadata.length());
            _commHelper.sendBlob (xMLMetadata.getBytes());
            _commHelper.write32 (data.length);
            _commHelper.sendBlob (data);

            int len = (dataMimeType == null ? 0 : dataMimeType.length());
            _commHelper.write32 (len);
            if (len > 0) {
                _commHelper.sendBlob (dataMimeType.getBytes());
    }

            _commHelper.write64 (expirationTime);
            _commHelper.write16 (historyWindow);
            _commHelper.write16 (tag);
            msgId = _commHelper.receiveLine();
            if (msgId == null || msgId.length() == 0) {
                throw new Exception ("makeAvailablePro_AVList failed");
            }
         
            _commHelper.sendLine ("OK");
            return msgId;
    	}
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    /**
     *
     * @param iClientId - the unique id of the application connecting to DSProp
     * @param groupName - the name of the group into which the message will be
     *                    published
     * @param metaDataAttributes - the array of the attributes that describe the
     *                             data; corresponding attribute-value elements
     *                             must be stored at the same position in the
     *                             proper array (ex: if the 3rd element of
     *                             metaDataAttributes is LATITUDE, the corresponding
     *                             value, say -79.33 will be stored as 3rd element
     *                             in metaDataValues).
     * @param metaDataValues -the array of the value of the attributes that describe
     *                        the data; corresponding attribute-value elements
     *                        must be stored at the same position in the proper
     *                        array (ex: if the 3rd element of metaDataAttributes
     *                        is LATITUDE, the corresponding value, say -79.33 will
     *                        be stored as 3rd element in metaDataValues).
     * @param data - the actual data of the message that has to be published.
     * @param dataMimeType - the MIME type of the data being published.
     * @param expirationTime - the length of time for which the published data is
     *                         considered relevant.  A value of 0 means that the
     *                         data is indefinitely relevant.
     * @param historyWindow - the number of messages prior of the current one
     *                        that are necessary to correctly interpreter the
     *                         message being published.
     * @param tag - tag is used to further specify the type on the message.
     * @return the id assigned to the published message, null in case of error.
     */
    public synchronized String makeAvailablePro (short iClientId, String  groupName, String objectId, String instanceId,
                                                 String[] metaDataAttributes, String[] metaDataValues, byte[] data, String dataMimeType,
                                                 long expirationTime, short historyWindow, short tag) throws CommException
    {
        checkConcurrentModification("makeAvailablePro - 1");

        String msgId;
    	try {
            _commHelper.sendLine ("makeAvailablePro_AVList");
            _commHelper.write16 (iClientId);
            _commHelper.sendLine (groupName);
            _commHelper.sendStringBlock (objectId);
            _commHelper.sendStringBlock (instanceId);

            _commHelper.write32 (metaDataAttributes.length);
            for (int i = 0; i < metaDataAttributes.length; i++) {
                if (metaDataAttributes != null) {
                    _commHelper.write32 (metaDataAttributes[i].length());
                    _commHelper.sendBlob (metaDataAttributes[i].getBytes());

                    if (metaDataValues[i] == null || metaDataValues[i].equals("")) {
                        // Sanity Check
                        // If no value is set, it has to be specified that the value
                        // is unknown
                        metaDataValues[i] = _UNKNOWN;
                    }
                    _commHelper.write32(metaDataValues[i].length());
                    _commHelper.sendBlob(metaDataValues[i].getBytes());
                }
            }

            _commHelper.write32 (data.length);
            _commHelper.sendBlob (data);

            int len = (dataMimeType == null ? 0 : dataMimeType.length());
            _commHelper.write32 (len);
            if (len > 0) {
                _commHelper.sendBlob (dataMimeType.getBytes());
            }

            _commHelper.write64 (expirationTime);
            _commHelper.write16 (historyWindow);
            _commHelper.write16 (tag);

            byte[] msgIdBuf = null;
            len = _commHelper.read32();
            if (len > 0) {
                msgIdBuf = _commHelper.receiveBlob (len);
            }
            if (msgIdBuf != null && msgIdBuf.length > 0) {
                msgId = new String (msgIdBuf);    
            }
            else {
                throw new Exception ("makeAvailablePro_AVList failed");
            }
            _commHelper.sendLine ("OK");
            return msgId;
        }
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            _LOGGER.log(Level.SEVERE, null, e);
            return null;
    	}
    }

    public boolean pathRegistered (NodePath path, String nodeId, String team, String mission)
    {
        synchronized (_listeners) {
            List<DisServiceProProxyListener> list = new LinkedList<DisServiceProProxyListener>();
            for (DisseminationServiceProxyListener l : _listeners.values()) {
                if (l instanceof DisServiceProProxyListener) {
                    list.add ((DisServiceProProxyListener)l);
                }
            }
            return pathRegistered (list, path, nodeId, team, mission);
        }
    }

    public static boolean pathRegistered (Collection<DisServiceProProxyListener> listeners, NodePath path, String nodeId, String team, String mission)
    {
        if (listeners.isEmpty()) {
            _LOGGER.severe ("DisServiceProProxy::pathRegistered: error:DisseminationServiceProxyListener is null");
            return false;
        }

        for (DisServiceProProxyListener listener : listeners) {
            listener.pathRegistered(path, nodeId, team, mission);
        }

        return true;
    }

    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId)
    {
        synchronized (_listeners) {
            List<DisServiceProProxyListener> list = new LinkedList<DisServiceProProxyListener>();
            for (DisseminationServiceProxyListener l : _listeners.values()) {
                if (l instanceof DisServiceProProxyListener) {
                    list.add ((DisServiceProProxyListener)l);
                }
            }
            return positionUpdated (list, latitude, longitude, altitude, nodeId);
        }
    }

    public static boolean positionUpdated (Collection<DisServiceProProxyListener> listeners, float latitude, float longitude, float altitude, String nodeId)
    {
        if (listeners.isEmpty()) {
            _LOGGER.warning ("DisServiceProProxy::pathRegistered: error:DisseminationServiceProxyListener is null");
            return false;
        }

        for (DisServiceProProxyListener listener : listeners) {
            listener.positionUpdated (latitude, longitude, altitude, nodeId);
        }

        return true;
    }

    public void informationMatched (String localNodeID, String peerNodeID,
                                    String skippedObjectID, String skippedObjectName,
                                    String[] rankDescriptors,
                                    float[] partialRanks, float[] weights,
                                    String comment, String operation)
    {
        synchronized (_matchmakerListeners) {
            if (_matchmakerListeners.isEmpty()) {
                _LOGGER.warning ("DisServiceProProxy::informationMatched: error:DisseminationServiceProxyListener is null");

            }

            for(int i = 0 ; i < _matchmakerListeners.size() ; i++) {
                _matchmakerListeners.get(i).informationMatched (localNodeID, peerNodeID,
                                                                skippedObjectID, skippedObjectName,
                                                                rankDescriptors,
                                                                partialRanks, weights,
                                                                comment, operation);
            }
        }
    }

    public void informationSkipped (String localNodeID, String peerNodeID,
                                    String skippedObjectID, String skippedObjectName,
                                    String[] rankDescriptors,
                                    float[] partialRanks, float[] weights,
                                    String comment, String operation)
    {
        synchronized (_matchmakerListeners) {
            if (_matchmakerListeners.isEmpty()) {
                _LOGGER.warning ("DisServiceProProxy::informationSkipped: error:DisseminationServiceProxyListener is null");
            }

            for (int i = 0 ; i < _matchmakerListeners.size() ; i++) {
                _matchmakerListeners.get(i).informationSkipped (localNodeID, peerNodeID,
                                                                skippedObjectID, skippedObjectName,
                                                                rankDescriptors,
                                                                partialRanks, weights,
                                                                comment, operation);
            }
        }
    }

    public final List<MatchmakingLogListener> _matchmakerListeners;

    // Constant for the SQLAVList
    // For how to use the SQLAVList see SQLAVList.h
    public static final String _INTEGER8 = "INTEGER8";
    public static final String _INTEGER16 = "INTEGER16";
    public static final String _INTEGER32 = "INTEGER32";
    public static final String _INTEGER64 = "INTEGER64";
    public static final String _FLOAT = "FLOAT";
    public static final String _DOUBLE = "DOUBLE";
    public static final String _TEXT = "TEXT";
    public static final String _UNKNOWN = "UNKNOWN";

    // Constant for to Path type
    public static final String _PAST_PATH = "Node Past Path";
    public static final int _MAIN_PATH_TO_OBJECTIVE = 1;
    public static final int _ALTERNATIVE_PATH_TO_OBJECTIVE = 2;
    public static final int _MAIN_PATH_TO_BASE = 3;
    public static final int _ALTERNATIVE_PATH_TO_BASE = 4;
    public static final int _FIXED_LOCATION = 5;

    private static final Logger _LOGGER = Logger.getLogger (DisServiceProProxy.class.getName());
}
