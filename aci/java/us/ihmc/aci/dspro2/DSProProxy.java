/*
 * DSProProxy.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import java.io.ByteArrayOutputStream;
import java.net.ConnectException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.naming.LimitExceededException;
import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.disServiceProProxy.PeerNodeContext;
import us.ihmc.aci.disServiceProxy.ConnectionStatusListener;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxy;
import us.ihmc.aci.dspro2.util.LoggerInterface;
import us.ihmc.aci.dspro2.util.LoggerWrapper;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.sync.ConcurrentProxy;
import us.ihmc.util.StringUtil;

/**
 * DS Pro Proxy
 *
 * @author Giacomo Benincasa and Maggie Breedy
 * @version $Revision: 1.114 $
 */
public class DSProProxy extends ConcurrentProxy implements CtrlMsgListener, MatchmakingLogListener,
        DSProProxyListener, SearchListener
{
    static class DSProProxyNotInitializedException extends RuntimeException {

        DSProProxyNotInitializedException(String msg) {
            super(msg);
        }

        private static void check(CommHelper ch) throws DSProProxyNotInitializedException {
            if (ch == null) {
                throw new DSProProxyNotInitializedException("DSProPrixy.init() should be called before invoking any other method");
            }
        }
    }

    private static final String PROTOCOL = "dspro";
    private static final String VERSION = "20160120";
    public static final String NO_REFERRED_OBJECT_ID = "NO_REF_OBJ";

    public DSProProxy()
    {
        super();
        _reinitializationAttemptInterval = 5000;
    }

    public DSProProxy (short applicationId)
    {
        _applicationId = applicationId;
        _reinitializationAttemptInterval = 5000;
    }

    /**
     * @param applicationId
     * @param reinitializationAttemptInterval: the time to wait before trying to
     *                                         reconnect to the DSPro service upon
     *                                         disconnection.
     */
    public DSProProxy (short applicationId, long reinitializationAttemptInterval)
    {
        _applicationId = applicationId;
        _reinitializationAttemptInterval = reinitializationAttemptInterval;
    }

    /**
     * @param applicationId
     * @param host: the IP of the disservicepro proxy server in dot-decimal
     *              notation
     * @param iPort
     */
    public DSProProxy (short applicationId, String host, int iPort)
    {
        _host = host;
        _port = iPort;
        _applicationId = applicationId;
    }

    /**
     * 
     * @param applicationId
     * @param host: the IP of the disservicepro proxy server in dot-decimal
     *              notation
     * @param port
     * @param reinitializationAttemptInterval: the time to wait before trying to
     *                                         reconnect to the DSPro service upon
     *                                         disconnection.
     */
    public DSProProxy (short applicationId, String host, int port, long reinitializationAttemptInterval)
    {
        _applicationId = applicationId;
        _reinitializationAttemptInterval = reinitializationAttemptInterval;
        _host = host;
        _port = port;
    }

    public void configLogger (String loggerConfig)
    {
        LOG.configure(loggerConfig);
    }

    public DisseminationServiceProxy getDisServiceProxy (String ip, int port) throws CommException
    {
        checkConcurrentModification("getDisServiceProxy");
        DSProProxyNotInitializedException.check(_commHelper);

        if (port < 0) {
            throw new IllegalArgumentException ("Negative port numbers are not allowed");
        }
        synchronized (this) {
            try {
                _commHelper.sendLine ("getDisService");
                _commHelper.write32 (port);
                _commHelper.sendStringBlock (ip);
                _commHelper.receiveMatch ("OK");

                DisseminationServiceProxy proxy = new DisseminationServiceProxy ((short) 0, ip, port);
                proxy.init();
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }

        return null;
    }
 
    public void addUserId (String userId) throws CommException
    {
        checkConcurrentModification("addUserId");
        DSProProxyNotInitializedException.check(_commHelper);
        if (userId == null) {
            return;
        }

        synchronized (this) {
            try {
                _commHelper.sendLine ("addUserId");
                _commHelper.sendStringBlock (userId);
                _commHelper.receiveMatch ("OK");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }
    }

    public void setMissionId (String missionId) throws CommException
    {
        checkConcurrentModification("setMissionId");
        DSProProxyNotInitializedException.check(_commHelper);

        if (missionId == null) {
            return;
        }

        synchronized (this) {
            try {
                _commHelper.sendLine ("setMissionId");
                _commHelper.sendStringBlock (missionId);
                _commHelper.receiveMatch ("OK");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }
    }

    public void setRole (String role) throws CommException
    {
        checkConcurrentModification("setRole");
        DSProProxyNotInitializedException.check(_commHelper);

        if (role == null) {
            return;
        }

        synchronized (this) {
            try {
                _commHelper.sendLine ("setRole");
                _commHelper.sendStringBlock (role);
                _commHelper.receiveMatch ("OK");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }
    }

    public void setDefaultUsefulDistance (int usefulDistanceInMeters) throws CommException
    {
        checkConcurrentModification("setDefUsefulDistance");
        DSProProxyNotInitializedException.check(_commHelper);

        if (usefulDistanceInMeters < 0) {
            throw new IllegalArgumentException("rangeOfInfluenceInMeters must be a positive number");
        }

        synchronized (this) {
            try {
                _commHelper.sendLine ("setDefUsefulDistance");
                _commHelper.writeUI32(usefulDistanceInMeters);
                _commHelper.receiveMatch ("OK");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }
    }

    public void setUsefulDistance (String dataMimeType, int usefulDistanceInMeters) throws CommException
    {
        checkConcurrentModification("setUsefulDistance");
        DSProProxyNotInitializedException.check(_commHelper);

        if (dataMimeType == null) {
            throw new IllegalArgumentException("milStd2525Symbol must not be null");
        }
        if (usefulDistanceInMeters < 0) {
            throw new IllegalArgumentException("rangeOfInfluenceInMeters must be a positive number");
        }

        synchronized (this) {
            try {
                _commHelper.sendLine ("setUsefulDistance");
                _commHelper.sendStringBlock (dataMimeType);
                _commHelper.writeUI32(usefulDistanceInMeters);
                _commHelper.receiveMatch ("OK");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }
    }

    public void setRangeOfInfluence (String milStd2525Symbol, int rangeOfInfluenceInMeters) throws CommException
    {
        checkConcurrentModification("setRangeOfInfluence");
        DSProProxyNotInitializedException.check(_commHelper);

        if (milStd2525Symbol == null) {
            throw new IllegalArgumentException("milStd2525Symbol must not be null");
        }
        if (rangeOfInfluenceInMeters < 0) {
            throw new IllegalArgumentException("rangeOfInfluenceInMeters must be a positive number");
        }

        synchronized (this) {
            try {
                _commHelper.sendLine ("setRangeOfInfluence");
                _commHelper.sendStringBlock (milStd2525Symbol);
                _commHelper.writeUI32(rangeOfInfluenceInMeters);
                _commHelper.receiveMatch ("OK");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }
    }

    public void setRankingWeights (float coordRankWeight, float timeRankWeight, float expirationRankWeight,
                                   float impRankWeight, float sourceReliabilityRankWeigth,
                                   float informationContentRankWeigth, float predRankWeight,
                                   float targetWeight, boolean bStrictTarget,
                                   boolean bConsiderFuturePathSegmentForMatchmacking) throws CommException
    {
        checkConcurrentModification("setRankingWeights");
        DSProProxyNotInitializedException.check(_commHelper);
        if (coordRankWeight < 0.0f) {
            throw new IllegalArgumentException("coordRankWeight must be a positive number");
        }
        if (timeRankWeight < 0.0f) {
            throw new IllegalArgumentException("timeRankWeight must be a positive number");
        }
        if (expirationRankWeight < 0.0f) {
            throw new IllegalArgumentException("expirationRankWeight must be a positive number");
        }
        if (impRankWeight < 0.0f) {
            throw new IllegalArgumentException("impRankWeight must be a positive number");
        }
        if (sourceReliabilityRankWeigth < 0.0f) {
            throw new IllegalArgumentException("sourceReliabilityRankWeigth must be a positive number");
        }
        if (informationContentRankWeigth < 0.0f) {
            throw new IllegalArgumentException("informationContentRankWeigth must be a positive number");
        }
        if (predRankWeight < 0.0f) {
            throw new IllegalArgumentException("predRankWeight must be a positive number");
        }
        if (targetWeight < 0.0f) {
            throw new IllegalArgumentException("targetWeight must be a positive number");
        }

        synchronized (this) {
            try {
                _commHelper.sendLine ("setRankingWeights");
                _commHelper.write32(coordRankWeight);
                _commHelper.write32(timeRankWeight);
                _commHelper.write32(expirationRankWeight);
                _commHelper.write32(impRankWeight);
                _commHelper.write32(sourceReliabilityRankWeigth);
                _commHelper.write32(informationContentRankWeigth);
                _commHelper.write32(predRankWeight);
                _commHelper.write32(targetWeight);
                _commHelper.writeBool(bStrictTarget);
                _commHelper.writeBool(bConsiderFuturePathSegmentForMatchmacking);
                _commHelper.receiveMatch ("OK");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }
    }

    public void addCustumPoliciesAsXML (Collection<String> customPoliciesXML) throws CommException
    {
        checkConcurrentModification("addCustumPoliciesAsXM");
        DSProProxyNotInitializedException.check(_commHelper);

        if ((customPoliciesXML == null) || (customPoliciesXML.size() <= 0)) {
            return;
        }

        synchronized (this) {
            try {
                _commHelper.sendLine ("addCustumPoliciesAsXM");
                int iCount = customPoliciesXML.size();
                if (iCount > Short.MAX_VALUE) {
                    throw new LimitExceededException ("addCustumPoliciesAsXML cannot add more than " + Short.MAX_VALUE + " policies at one time.");
                }
                _commHelper.write16 ((short) iCount);
                for (String policy : customPoliciesXML) {
                    _commHelper.sendStringBlock (policy);
                }
                _commHelper.receiveMatch ("OK");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }
    }

    public String getSessionId() throws CommException
    {
        checkConcurrentModification("getSessionId");
        DSProProxyNotInitializedException.check(_commHelper);

        synchronized (this) {
            try {
                _commHelper.sendLine("getSessionId");
                _commHelper.receiveMatch("OK");

                int idLen = _commHelper.read32();
                if (idLen > 0) {
                    byte[] buf = new byte[idLen];
                    _commHelper.receiveBlob (buf, 0, idLen);
                    if (buf == null || buf.length == 0) {
                        _commHelper.sendLine("ERROR");
                    }
                    else {
                        _commHelper.sendLine("OK");
                    }

                    return new String(buf);
                }
                else {
                    _commHelper.sendLine("OK");
                }
                return null;
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set(false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }

        return null;
    }

    /**
     * Connect to the service.
     * @return 0 if the connection to the service was successful, a negative
     * number otherwise.
     */
    public synchronized int init()
    {
        if (_host == null) {
            _host = "127.0.0.1";
        }
        if (_port == 0) {
            _port = DIS_SVC_PROXY_SERVER_PORT_NUMBER;
        }
        LOG.info ("Getting server  host: " + _host + "   port: " + _port);
        try {
            CommHelper ch = connectToServer(_host, _port);
            CommHelper chCallback = connectToServer(_host, _port);
            if (ch != null || chCallback != null) {
                doHandshake (ch);
                doHandshake (chCallback);
                int rc = registerProxy(ch, chCallback, _applicationId);
                if (rc < 0) {
                    return -1;
                }
                else {
                    _applicationId = (short) rc; // The server may have assigned
                    // a different id than requested
                }
                _commHelper = ch;
                _handler = new DSProProxyCallbackHandler(this, chCallback);
                _handler.start();
            }
        }
        catch (ConnectException e) {
            LOG.warn(e.getMessage());
            return -1;
        }
        catch (Exception e) {
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return -1;
        }

        _isInitialized.set (true);
        return 0;
    }

    /**
     * Returns true if DSProProxy successfully connected to the DSPro service,
     * and if it still connected.
     * @return 
     */
    public boolean isInitialized()
    {
        return _isInitialized.get();
    }

    /**
     * Add metadata annotation to the object identified by pszReferredObject.
     * This metadata annotation is sent to the nodes which node context
     * matches the metadata annotation.
     *
     * @param groupName: analogous to addMessage()
     * @param objectId: analogous to addMessage()
     * @param instanceId: analogous to addMessage()
     * @param xmlMedatada: analogous to addMessage()
     * @param referredObject: analogous to addMessage()
     * @param expirationTime: analogous to addMessage()
     * @return
     * @throws CommException 
     * @throws us.ihmc.aci.dspro2.DSProProxy.DSProProxyNotInitializedException 
     */
    public String addAnnotation (String groupName, String objectId, String instanceId,
                                 String xmlMedatada, String referredObject,
                                 long expirationTime)
            throws CommException
    {
        checkConcurrentModification("addAnnotation");
        DSProProxyNotInitializedException.check(_commHelper);

        synchronized (this) {
            String id = null;
            try {
                _commHelper.sendLine("addAnnotation");
                _commHelper.sendLine(groupName);
                _commHelper.sendStringBlock(objectId);
                _commHelper.sendStringBlock(instanceId);
                _commHelper.write32(xmlMedatada.length());
                _commHelper.sendBlob(xmlMedatada.getBytes());
                _commHelper.write32(referredObject.length());
                _commHelper.sendBlob(referredObject.getBytes());
                _commHelper.write64(expirationTime);

                _commHelper.sendLine("OK");

                byte[] bId = _commHelper.receiveBlock();
                if (bId != null) {
                    id = new String (bId);
                }
                else {
                    id = null;
                }
                _commHelper.sendLine("OK");

                return id;
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set(false);
                    throw (CommException) e;
                }
                LOG.warn(StringUtil.getStackTraceAsString(e));
                return id;
            }
        }
    }

    /**
     * Add metadata annotation to the object identified by pszReferredObject.
     * This metadata annotation is sent to the nodes which node context
     * matches the metadata annotation.
     *
     * @param groupName
     * @param objectId
     * @param instanceId
     * @param xmlMetadata
     * @param expirationTime
     * @return
     * @throws CommException 
     */
    public String addAnnotation (String groupName, String objectId, String instanceId,
                                 String xmlMetadata, long expirationTime)
            throws CommException
    {
        checkConcurrentModification("addAnnotation");
        DSProProxyNotInitializedException.check(_commHelper);

        synchronized (this) {
            String id = null;
            try {
                _commHelper.sendLine("addAnnotation");
                _commHelper.sendLine(groupName);
                _commHelper.sendStringBlock(objectId);
                _commHelper.sendStringBlock(instanceId);
                _commHelper.write32(xmlMetadata.length());
                _commHelper.sendBlob(xmlMetadata.getBytes());
                _commHelper.write64(expirationTime);

                _commHelper.sendLine("OK");

                id = new String(_commHelper.receiveBlock());
                _commHelper.sendLine("OK");

                return id;
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set(false);
                    throw (CommException) e;
                }
                LOG.warn(StringUtil.getStackTraceAsString(e));
                return id;
            }
        }
    }

    /**
     * The message is stored and its metadata is sent to the nodes which
     * node context matches the metadata describing the data.
     * 
     * @param groupName: the group of the message.
     * @param objectId
     * @param instanceId
     * @param xmlMedatada: string that containing and XML document describing
     *                     the data
     * @param data: the actual data of the message.
     * @param expirationTime: the expiration time of the message.
     *                        (if set to 0, the message never expires)
     * @return
     * @throws CommException 
     */
    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                                           String xmlMedatada, byte[] data, long expirationTime)
            throws CommException
    {
        checkConcurrentModification("addMessage");
        DSProProxyNotInitializedException.check(_commHelper);
        String id = null;

        try {
            _commHelper.sendLine("addMessage");
            _commHelper.sendLine(groupName);
            _commHelper.sendStringBlock(objectId);
            _commHelper.sendStringBlock(instanceId);

            _commHelper.write32(xmlMedatada.length());
            _commHelper.sendBlob(xmlMedatada.getBytes());
            if (data == null) {
                _commHelper.write32(0);
            }
            else {
                _commHelper.write32(data.length);
                _commHelper.sendBlob(data);
            }
            _commHelper.write64(expirationTime);

            _commHelper.receiveMatch("OK");

            byte[] bId = _commHelper.receiveBlock();
            if (bId != null) {
                id = new String (bId);
            }
            else {
                id = null;
            }
            _commHelper.sendLine("OK");

            return id;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return id;
        }
    }

    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                                           Map<Object, Object> metadata, byte[] data, long expirationTime)
            throws CommException
    {
        checkConcurrentModification("addMessage_AVList");
        DSProProxyNotInitializedException.check(_commHelper);
        String id = null;
        try {
            _commHelper.sendLine("addMessage_AVList");
            _commHelper.sendLine(groupName);
            _commHelper.sendStringBlock(objectId);
            _commHelper.sendStringBlock(instanceId);

            Set<Entry<Object, Object>> entries = metadata.entrySet();
            _commHelper.write32(entries.size());
            for (Entry<Object, Object> entry : entries) {
                String attribute = entry.getKey().toString();
                _commHelper.write32(attribute.length());
                _commHelper.sendBlob(attribute.getBytes());

                String value = entry.getValue().toString();
                if (value == null || value.equals("")) {
                    // Sanity Check
                    // If no value is set, it has to be specified that the value
                    // is unknown
                    value = _UNKNOWN;
                }
                _commHelper.write32(value.length());
                _commHelper.sendBlob(value.getBytes());
            }

            if (data == null) {
                _commHelper.write32(0);
            }
            else {
                _commHelper.write32(data.length);
                _commHelper.sendBlob(data);
            }
            _commHelper.write64(expirationTime);

            _commHelper.receiveMatch("OK");

            byte[] bId = _commHelper.receiveBlock();
            if (bId != null) {
                id = new String (bId);
            }
            else {
                id = null;
            }
            
            return id;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return id;
        }
    }

    /**
     * The message is stored and its metadata is sent to the nodes which
     * node context matches the metadata describing the data.
     *
     * @deprecated
     * @param groupName: the group of the message.
     * @param objectId
     * @param instanceId
     * @param metaDataAttributes: string array containing the list of metadata
     *                            attributes being set
     * @param metaDataValues: string array containing the list of metadata
     *                        values, corresponding to the metadata attributed
     *                        in metaDataAttributes, that are being set.
     * @param data: the actual data of the message.
     * @param expirationTime: the expiration time of the message.
     *                        (if set to 0, the message never expires)
     * @return
     * @throws CommException
     */
    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                                           String[] metaDataAttributes, String[] metaDataValues,
                                           byte[] data, long expirationTime)
            throws CommException
    {
        checkConcurrentModification("addMessage_AVList");
        DSProProxyNotInitializedException.check(_commHelper);
        String id = null;
        try {
            _commHelper.sendLine("addMessage_AVList");
            _commHelper.sendLine(groupName);
            _commHelper.sendStringBlock(objectId);
            _commHelper.sendStringBlock(instanceId);

            _commHelper.write32(metaDataAttributes.length);
            for (int i = 0; i < metaDataAttributes.length; i++) {
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
            if (data == null) {
                _commHelper.write32(0);
            }
            else {
                _commHelper.write32(data.length);
                _commHelper.sendBlob(data);
            }
            _commHelper.write64(expirationTime);

            try {
                _commHelper.receiveMatch("OK");
                byte[] bId = _commHelper.receiveBlock();
                if (bId != null) {
                    id = new String (bId);
                }
                else {
                    id = null;
                }
            }
            catch (ProtocolException pe) {
                LOG.warn(StringUtil.getStackTraceAsString(pe));
            }

            return id;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return id;
        }
    }

    /**
     * Connect to a peer that is listening pszRemoteAddr:ui16Port,
     * using an adaptor of type "type.
     *
     * NOTE: currently, the only supporter adaptor is AdaptorType::MOCKETS
     * @param adaptorType
     * @param networkInterface
     * @param remoteAddress
     * @param port
     * @return 
     * @throws us.ihmc.comm.CommException 
     */
    public synchronized boolean addPeer (short adaptorType, String networkInterface, String remoteAddress,
                                         short port)
            throws CommException
    {
        checkConcurrentModification("addPeer");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("addPeer");
            if (adaptorType > 0) {
                _commHelper.write16(adaptorType);
            }
            _commHelper.write32(networkInterface.length());
            _commHelper.sendBlob(networkInterface.getBytes());

            _commHelper.write32(remoteAddress.length());
            _commHelper.sendBlob(remoteAddress.getBytes());
            _commHelper.write16(port);

            _commHelper.receiveMatch("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return false;
        }
    }

    /**
     * Cancel a message from the DSPro cache.
     *
     * @param id: the ID of the message to cancel
     * @throws CommException
     * @throws ProtocolException 
     */
    public synchronized void cancel (String id)
            throws CommException, ProtocolException
    {
        checkConcurrentModification("cancel - 0");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("cancel_str");
            _commHelper.sendLine(id);

            _commHelper.receiveMatch("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            if (e instanceof ProtocolException) {
                throw (ProtocolException) e;
            }
            LOG.error(e);
        }
    }

    /**
     * If the data of the message is of one of the supported MIME types,
     * the message is chunked in smaller, individually intelligible, but
     * containing lower resolution or incomplete data, messages.  These
     * messages are then stored and the metadata is sent to the nodes which
     * node context matches the metadata describing the data.
     * The node receiving the metadata can retrieve individual chunks.
     * If the MIME type of the data is not supported, then chunkAndAddMessage()
     * behaves as addMessage().
     *
     * @param groupName: the group of the message.
     * @param objectId
     * @param instanceId
     * @param xmlMedatada: string that containing and XML document describing
     *                     the data
     * @param data: the actual data of the message.
     * @param dataMimeType: the MIME type of the data.
     * @param expirationTime: the expiration time of the message.
     *                        (if set to 0, the message never expires)
     * @return
     * @throws CommException
     */
    public synchronized String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                                   String xmlMedatada, byte[] data, String dataMimeType,
                                                   long expirationTime)
            throws CommException
    {
        checkConcurrentModification("chunkAndAddMessage");
        DSProProxyNotInitializedException.check(_commHelper);

        String id = null;
        try {
            _commHelper.sendLine("chunkAndAddMessage");
            _commHelper.sendLine(groupName);
            _commHelper.sendStringBlock(objectId);
            _commHelper.sendStringBlock(instanceId);
            _commHelper.write32(xmlMedatada.length());
            _commHelper.sendBlob(xmlMedatada.getBytes());
            _commHelper.write32(data.length);
            _commHelper.sendBlob(data);
            _commHelper.write32(dataMimeType.length());
            _commHelper.sendBlob(dataMimeType.getBytes());
            _commHelper.write64(expirationTime);

            _commHelper.receiveMatch("OK");

            byte[] bId = _commHelper.receiveBlock();
            if (bId != null) {
                id = new String (bId);
            }
            else {
                id = null;
            }
            _commHelper.sendLine("OK");

            return id;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return id;
        }
    }

    /**
     * If the data of the message is of one of the supported MIME types,
     * the message is chunked in smaller, individually intelligible, but
     * containing lower resolution or incomplete data, messages.  These
     * messages are then stored and the metadata is sent to the nodes which
     * node context matches the metadata describing the data.
     * The node receiving the metadata can retrieve individual chunks.
     * If the MIME type of the data is not supported, then chunkAndAddMessage()
     * behaves as addMessage().
     *
     * @param groupName: the group of the message.
     * @param objectId
     * @param instanceId
     * @param metaDataAttributes: string array containing the list of metadata
     *                            attributes being set
     * @param metaDataValues: string array containing the list of metadata
     *                        values, corresponding to the metadata attributed
     *                        in metaDataAttributes, that are being set.
     * @param data: the actual data of the message.
     * @param dataMimeType: the MIME type of the data.
     * @param expirationTime: the expiration time of the message.
     *                        (if set to 0, the message never expires)
     * @return
     * @throws CommException
     */
    public synchronized String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                                   String[] metaDataAttributes, String[] metaDataValues,
                                                   byte[] data, String dataMimeType,
                                                   long expirationTime) throws CommException
    {
        checkConcurrentModification ("chunkAndAddMessage_AVList");
        DSProProxyNotInitializedException.check(_commHelper);
        String id = null;
        try {
            _commHelper.sendLine ("chunkAndAddMessage_AVList");
            _commHelper.sendLine(groupName);
            _commHelper.sendStringBlock(objectId);
            _commHelper.sendStringBlock(instanceId);
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
            _commHelper.write32(data.length);
            _commHelper.sendBlob(data);
            _commHelper.write32(dataMimeType.length());
            _commHelper.sendBlob(dataMimeType.getBytes());
            _commHelper.write64(expirationTime);

            _commHelper.receiveMatch ("OK");

            byte[] bId = _commHelper.receiveBlock();
            if (bId != null) {
                id = new String (bId);
            }
            else {
                id = null;
            }

            return id;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return id;
        }
    }

    /**
     * Overrides DSPro service configuration parameters at run time.
     * @param properties
     * @return
     * @throws CommException 
     */
    public synchronized boolean configureProperties (Properties properties)
        throws CommException
    {
        checkConcurrentModification("configureProperties");
        DSProProxyNotInitializedException.check(_commHelper);

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        try {
            _commHelper.sendLine("configureProperties");
            properties.store(out, null);
            _commHelper.write32(out.size());
            _commHelper.sendBlob(out.toByteArray());

            _commHelper.receiveMatch("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
        }

        return false;
    }

    public synchronized boolean setRankingWeights (float coordRankWeight, float timeRankWeight,
                                                   float expirationRankWeight, float impRankWeight,
                                                   float predRankWeight, float targetWeight,
                                                   boolean strictTarget) throws CommException
    {
        checkConcurrentModification("setRankingWeights");
        DSProProxyNotInitializedException.check(_commHelper);

        try {

            // TODO: implement this
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return false;
        }
    }

    /**
     * Adds a path to DSPro.  The path is NOT set as the current path, until
     * setCurrentPath() is called.
     * @param path
     * @return 
     * @throws us.ihmc.comm.CommException
     */
    public synchronized boolean registerPath (NodePath path)
            throws CommException
    {
        checkConcurrentModification("registerPath");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("registerPath");

            if (!path.write(_commHelper)) {
                LOG.warn("DisseminationServiceProProxy-registerPath: Failed to send the path");
                return false;
            }

            _commHelper.receiveMatch("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return false;
        }
    }

    /**
     * @return all the cached metadata messages
     * @throws CommException 
     */
    public synchronized List<String> getAllCachedMetaDataAsXML()
            throws CommException
    {
        return getAllCachedMetaDataAsXML(0, 0);
    }

    /**
     * @param startTimestamp
     * @param endTimestamp
     * @return returns the metadata messages whose metadata whose SOURCE_TIME is
     * within i64BeginArrivalTimestamp and i64EndArrivalTimestamp.
     * @throws CommException 
     */
    public synchronized List<String> getAllCachedMetaDataAsXML (long startTimestamp, long endTimestamp)
            throws CommException
    {
        return getMatchingMetaDataAsXML(new HashMap<String, String>(), 0, 0);
    }

    /**
     * Returns the path that is currently set.
     */
    public synchronized NodePath getCurrentPath()
        throws CommException
    {
        checkConcurrentModification("getCurrentPath");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("getCurrentPath");
            _commHelper.receiveMatch("OK");

            NodePath newPath = NodePath.read(_commHelper);

            _commHelper.sendLine("OK");
            return newPath;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return null;
        }
    }

    /**
     * @param peerNodeId
     * @return the path for the peer identified by peerNodeId
     * @throws CommException 
     */
    public synchronized NodePath getPathForPeer (String peerNodeId)
            throws CommException
    {
        if (peerNodeId == null || peerNodeId.length() == 0) {

        }
        checkConcurrentModification("getPathForPeer");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("getPathForPeer");

            _commHelper.sendLine(peerNodeId);
            String match = _commHelper.receiveLine();
            NodePath newPath = null;
            if (match.equalsIgnoreCase("NOPATH")) {
                return null;
            }
            else if (match.equalsIgnoreCase("OK")) {
                newPath = NodePath.read(_commHelper);
            }
            else {
                throw new ProtocolException("no match found: looked for OK or NOPATH in {" + match + "}");
            }

            _commHelper.sendLine("OK");
            return newPath;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return null;
        }
    }

    /**
     * Returns the metadata messages whose metadata attributes match
     * the ones specified in attributeValueToMatch.
     *
     * @param attributeValueToMatch: the set of attributes to match, and the
     *                               corresponding values
     * @return a list of XML documents containing the metadata message
     * @throws CommException 
     */
    public synchronized List<String> getMatchingMetaDataAsXML (Map<String, String> attributeValueToMatch)
            throws CommException
    {
        return getMatchingMetaDataAsXML(attributeValueToMatch, 0, 0);
    }

    /**
     * Returns the metadata messages whose metadata attributes match
     * the ones specified in attributeValueToMatch, and whose SOURCE_TIME is
     * within i64BeginArrivalTimestamp and i64EndArrivalTimestamp.
     * If i64BeginArrivalTimestamp and i64EndArrivalTimestamp are both
     * set to 0, then no constraint is specified on SOURCE_TIME.
     *
     * @param attributeValueToMatch: the set of attributes to match, and the
     *                               corresponding values
     * @param startTimestamp
     * @param endTimestamp
     * @return a list of XML documents containing the metadata message
     * @throws CommException 
     */
    public synchronized List<String> getMatchingMetaDataAsXML (Map<String, String> attributeValueToMatch,
                                                               long startTimestamp, long endTimestamp)
            throws CommException
    {
        checkConcurrentModification("getMatchingMetaDataAsXML");
        DSProProxyNotInitializedException.check(_commHelper);

        List<String> metadata = new LinkedList<String>();
        try {
            _commHelper.sendLine("getMatchingMetaDataAsXML");
            _commHelper.write32(attributeValueToMatch.size());
            for (Map.Entry<String, String> av : attributeValueToMatch.entrySet()) {
                _commHelper.sendStringBlock(av.getKey());
                _commHelper.sendStringBlock(av.getValue());
            }

            _commHelper.write64(startTimestamp);
            _commHelper.write64(endTimestamp);

            int metadataLen = _commHelper.read32();
            while (metadataLen > 0) {
                metadata.add(new String(_commHelper.receiveBlob(metadataLen)));
                metadataLen = _commHelper.read32();
            }

            _commHelper.receiveMatch("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(e.getMessage());
            return null;
        }

        return metadata;
    }

    /**
     * Sets an existing path, as the current path. The initial position is set
     * to the initial point in the path.
     * @param pathID
     * @return 
     * @throws us.ihmc.comm.CommException 
     */
    public synchronized boolean setCurrentPath (String pathID)
            throws CommException
    {
        checkConcurrentModification("setCurrentPath");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("setCurrentPath");
            _commHelper.sendLine(pathID);

            _commHelper.receiveMatch("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return false;
        }
    }

    /**
     * Set the current position of the node.
     * @param fLatitude
     * @param fLongitude
     * @param fAltitude
     * @param location
     * @param note
     * @return 
     * @throws us.ihmc.comm.CommException 
     */
    public synchronized boolean setCurrentPosition (float fLatitude, float fLongitude, float fAltitude,
                                                    String location, String note)
            throws CommException
    {
        checkConcurrentModification("setCurrentPosition");
        DSProProxyNotInitializedException.check(_commHelper);

        if (location == null) {
            location = "Default";
        }
        if (note == null) {
            note = "Default";
        }

        try {
            _commHelper.sendLine("setCurrentPosition");
            _commHelper.sendLine(Float.toString(fLatitude));
            _commHelper.sendLine(Float.toString(fLongitude));
            _commHelper.sendLine(Float.toString(fAltitude));
            _commHelper.sendBlock(location.getBytes());
            _commHelper.sendBlock(note.getBytes());

            _commHelper.receiveMatch("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return false;
        }
    }

    /**
     * Request a high resolution area of the original chunk.
     * The origin of the axis is in the bottom left corner.
     *
     * @param chunkedMsgId: the DSPro message ID of the original message
     * @param mimeType: the MIME type of the original message
     * @param ui32StartXPixel: the index of the first pixel of the requested custom chunk, on the x axis
     * @param ui32EndXPixel: the index of the last pixel of the requested custom chunk, on the x axis
     * @param ui32StartYPixel: the index of the first pixel of the requested custom chunk, on the y axis
     * @param ui32EndYPixel: the index of the last pixel of the requested custom chunk, on the y axis
     * @param ui8CompressionQuality
     * @param timeoutInMilliseconds: the time after which DSPro will stop searching for the message.
     *                               If timeoutInMilliseconds is set to 0 DSPro continues the search
     *                               indefinitely.
     * @throws CommException 
     */
    public synchronized void requestCustomAreaChunk (String chunkedMsgId, String mimeType,
                                 long ui32StartXPixel, long ui32EndXPixel,
                                 long ui32StartYPixel, long ui32EndYPixel,
                                 byte ui8CompressionQuality, long timeoutInMilliseconds)
            throws CommException
    {
        checkConcurrentModification("requestCustomAreaChunk");
        DSProProxyNotInitializedException.check(_commHelper);
        if ((ui32StartXPixel < 0) || (ui32EndXPixel < 0) || (ui32StartYPixel < 0) || (ui32EndYPixel < 0) || (ui8CompressionQuality < 0)) {
            throw new IllegalArgumentException();
        }
        try {
            _commHelper.sendLine("requestCustomAreaChunk");
            _commHelper.sendStringBlock(chunkedMsgId);
            _commHelper.sendStringBlock(mimeType);
            _commHelper.writeUI32 (ui32StartXPixel);
            _commHelper.writeUI32 (ui32EndXPixel);
            _commHelper.writeUI32 (ui32StartYPixel);
            _commHelper.writeUI32 (ui32EndYPixel);
            _commHelper.write8(ui8CompressionQuality);
            _commHelper.write64(timeoutInMilliseconds);
            _commHelper.receiveMatch("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
        }
    }

    /**
     * Request a high resolution portion of the original chunk.
     *
     * @param chunkedMsgId: the DSPro message ID of the original message
     * @param mimeType: the MIME type of the original message
     * @param startTime: the time when the portion starts
     * @param endTime: the time when the portion ends
     * @param ui8CompressionQuality
     * @param timeoutInMilliseconds: the time after which DSPro will stop searching for the message.
     *                               If timeoutInMilliseconds is set to 0 DSPro continues the search
     *                               indefinitely.
     * @throws CommException 
     */
    public synchronized void requestCustomTimeChunk (String chunkedMsgId, String mimeType,
                                                     long startTime, long endTime,
                                                     byte ui8CompressionQuality, long timeoutInMilliseconds)
            throws CommException
    {
        checkConcurrentModification("requestCustomTimeChunk");
        DSProProxyNotInitializedException.check(_commHelper);
        if ((startTime < 0) || (endTime < 0) || (ui8CompressionQuality < 0)) {
            throw new IllegalArgumentException();
        }
        try {
            _commHelper.sendLine("requestCustomTimeChunk");
            _commHelper.sendStringBlock(chunkedMsgId);
            _commHelper.sendStringBlock(mimeType);
            _commHelper.write64(startTime);
            _commHelper.write64 (endTime);
            _commHelper.write8(ui8CompressionQuality);
            _commHelper.write64(timeoutInMilliseconds);
            _commHelper.receiveMatch("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
        }
    }

    public synchronized boolean requestMoreChunks (String messageId)
            throws CommException
    {
        return requestMoreChunks (messageId, null);
    }

    /**
     * Request more chunks for the message identified by messageId.
     * The chunks will be delivered asynchronously via the dataArrived()
     * callback in DSProProxyListener.
     * @param messageId
     * @param callbackParameters
     * @return 
     * @throws us.ihmc.comm.CommException 
     */
    public synchronized boolean requestMoreChunks (String messageId, String callbackParameters)
            throws CommException
    {
        checkConcurrentModification("requestMoreChunks");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("requestMoreChunks");
            _commHelper.sendLine(messageId);
            _commHelper.sendStringBlock(callbackParameters);

            _commHelper.receiveMatch("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.error(e);
        }

        return true;
    }

    /**
     * Set the battery level of the node.
     *
     * NOTE: this feature is not yet supported.
     * @param batteryLevel
     * @return 
     * @throws us.ihmc.comm.CommException
     */
    public synchronized boolean setBatteryLevel (int batteryLevel)
            throws CommException
    {
        checkConcurrentModification("setBatteryLevel");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("setBatteryLevel");
            _commHelper.write32(batteryLevel);

            _commHelper.receiveMatch("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return false;
        }
    }

    /**
     * Set the memory available on the node.
     *
     * NOTE: this feature is not yet supported.
     * @param memoryAvailable
     * @return 
     * @throws us.ihmc.comm.CommException 
     */
    public synchronized boolean setMemoryAvailable (int memoryAvailable)
            throws CommException
    {
        checkConcurrentModification("setMemoryAvailable");
        DSProProxyNotInitializedException.check(_commHelper);
        try {
            _commHelper.sendLine("setBatteryLevel");
            _commHelper.write32(memoryAvailable);

            _commHelper.receiveMatch("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return false;
        }

    }

    public synchronized byte getAdaptorType (short adaptorId)
            throws CommException
    {
        checkConcurrentModification("getAdaptorType");
        DSProProxyNotInitializedException.check(_commHelper);
        try {
            _commHelper.sendLine("getAdaptorType");
            _commHelper.write16(adaptorId);

            _commHelper.receiveMatch("OK");

            return _commHelper.read8();

        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return 0;
        }
    }

    /**
     * Returns the given DSProId for the object with the given objectId and instanceId,
     * if present, null otherwise.
     *
     * @param objectId
     * @param instanceId
     * @return
     * @throws CommException
     */
    public synchronized String getDSProId (String objectId, String instanceId)
            throws CommException
    {
        if (objectId == null || instanceId == null) {
            return null;
        }

        checkConcurrentModification("getDSProId");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("getDSProId");
            _commHelper.sendStringBlock(objectId);
            _commHelper.sendStringBlock(instanceId);

            _commHelper.receiveMatch("OK");

            List<String> list = new LinkedList<String>();
            byte[] b;
            while ((b = _commHelper.receiveBlock()) != null) {
                list.add (new String (b));
            }

            _commHelper.receiveMatch("OK");
            return list.isEmpty() ? null : list.get (0);
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return null;
        }
    }

    /**
     * Returns the data identified by pszId. If the data is not
     * currently available in the local data cache, null is returned.
     * If the data ever arrives at a later point, it is asynchronously
     * returned via the dataArrived() callback in DSProListener.
     * @param messageID
     * @return 
     * @throws us.ihmc.comm.CommException
     */
    public synchronized DataWrapper getData (String messageID)
            throws CommException
    {
        return getData (messageID, null);
    }

    public synchronized DataWrapper getData (String messageID, String callbackParameters)
            throws CommException
    {
        checkConcurrentModification("getData");
        DSProProxyNotInitializedException.check(_commHelper);

        if (NO_REFERRED_OBJECT_ID.compareToIgnoreCase (messageID) == 0) {
            throw new IllegalArgumentException (messageID + " is not a valid DSPro ID.");
        }

        try {
            _commHelper.sendLine("getData");
            _commHelper.sendLine(messageID);
            _commHelper.sendStringBlock(callbackParameters);

            _commHelper.receiveMatch("OK");

            int idataLength = _commHelper.read32();
            byte[] data = null;
            byte hasMoreChunks = 0;
            if (idataLength > 0) {
                LOG.info("Retrieving data of length " + idataLength);
                data = _commHelper.receiveBlob(idataLength);
                hasMoreChunks = _commHelper.read8();  //need more
            }

            _commHelper.sendLine("OK");

            if (data == null) {
                return null;
            }
            return new DataWrapper(data, (hasMoreChunks == 1), callbackParameters);
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return null;
        }
    }

    /**
     * Give feedback about the usefulness of the data identified by pszMessageID.
     *
     * NOTE: not yet implemented
     * @param messageID
     * @throws us.ihmc.comm.CommException
     */
    public synchronized void notUseful (String messageID)
            throws CommException
    {
        checkConcurrentModification("notUseful");
        DSProProxyNotInitializedException.check(_commHelper);

        if (messageID == null) {
            throw new NullPointerException();
        }

        //Not implemented yet
    }

    public synchronized String search (String groupName, String queryType,
                                       String queryQualifiers, byte[] query)
        throws CommException
    {
        return search (groupName, queryType, queryQualifiers, query, (long) 0);
    }

    /**
     * Search data based on the given groupName and a query on the
     * metadata fields. The method returns the query ID, or null in case
     * of error.
     *
     * @param groupName a group name for the search
     * @param queryType the type of the query. It is used to identify the proper
     *                  search controller to handle the query
     * @param queryQualifiers the query itself
     * @param query
     * @param timeoutInMilliseconds the time after which DSPro will stop searching
     *                              for the message. If timeoutInMilliseconds is
     *                              set to 0 DSPro continues the search indefinitely.
     * @return
     * @throws CommException 
     */
    public synchronized String search (String groupName, String queryType,
                                       String queryQualifiers, byte[] query,
                                       long timeoutInMilliseconds)
            throws CommException
    {
        checkConcurrentModification("search");
        DSProProxyNotInitializedException.check(_commHelper);

        if (groupName == null || query == null) {
            throw new NullPointerException();
        }

        try {
            _commHelper.sendLine("search");
            _commHelper.sendStringBlock(groupName);
            _commHelper.sendStringBlock(queryType);
            _commHelper.sendStringBlock(queryQualifiers);
            if (query == null || query.length <= 0) {
                throw new IllegalArgumentException("The query can't be null");
            }
            _commHelper.write32(query.length);
            _commHelper.sendBlob(query);
            _commHelper.write64 (timeoutInMilliseconds);

            _commHelper.receiveMatch("OK");

            // read the ID assigned to the query
            byte[] b = _commHelper.receiveBlock();
            String queryId = b != null ? new String(b) : null;

            return queryId;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return null;
        }
    }

    /**
     * Method to return the list of IDs that match the query identified
     * by pszQueryId.
     *
     * @param queryId: the id of the matched query
     * @param disServiceMsgIds: the list of matching messages IDs
     * @throws CommException 
     */
    public synchronized void replyToSearch (String queryId, Collection<String> disServiceMsgIds)
            throws CommException
    {
        checkConcurrentModification("replyToQuery");
        DSProProxyNotInitializedException.check(_commHelper);

        if (queryId == null || disServiceMsgIds == null) {
            throw new NullPointerException();
        }

        try {
            // Write query id
            _commHelper.sendLine("replyToQuery");
            _commHelper.sendStringBlock(queryId);

            // Write number of elements in disServiceMsgIds
            _commHelper.write32(disServiceMsgIds.size());

            // Write Ids
            for (String msgId : disServiceMsgIds) {
                _commHelper.sendStringBlock(msgId);
            }

            _commHelper.receiveMatch("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
        }
    }

    public synchronized void replyToQuery (String queryId, byte[] reply)
        throws CommException
    {
        checkConcurrentModification("volatileReplyToQuery");
        DSProProxyNotInitializedException.check(_commHelper);

        if (queryId == null || reply == null) {
            throw new NullPointerException();
        }

        try {
            // Write query id
            _commHelper.sendLine("volatileReplyToQuery");
            _commHelper.sendStringBlock(queryId);

            _commHelper.sendBlock (reply);

            _commHelper.receiveMatch("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
        }
    }

    /**
     * The transmission history maintains a list of the replicated
     * messages for each peer, in order to avoid replicating the same
     * message multiple times.
     * resetTransmissionHistory() can be used to reset it, if even
     * necessary.
     *
     * NOTE: this method is not yet implemented.
     * @return 
     * @throws us.ihmc.comm.CommException
     */
    public boolean resetTransmissionHistory() throws CommException
    {
        checkConcurrentModification("resetTransmissionHistory");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("resetTransmissionHistory");
            _commHelper.receiveMatch("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.error(e);
            return false;
        }
    }

    /***
     *
     * @return: the node id of the node
     * @throws CommException 
     */
    public String getNodeId() throws CommException
    {
        checkConcurrentModification("getNodeId");
        DSProProxyNotInitializedException.check(_commHelper);
        if (_nodeId != null) {
            return _nodeId;
        }

        synchronized (this) {
            try {
                _commHelper.sendLine("getNodeId");
                _commHelper.receiveMatch("OK");

                int idLen = _commHelper.read32();
                if (idLen > 0) {
                    byte[] buf = new byte[idLen];
                    _commHelper.receiveBlob(buf, 0, idLen);
                    if (buf == null || buf.length == 0) {
                        _commHelper.sendLine("ERROR");
                    }
                    else {
                        _commHelper.sendLine("OK");
                    }

                    return new String(buf);
                }
                else {
                    _commHelper.sendLine("OK");
                }
                return null;
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set(false);
                    throw (CommException) e;
                }
                LOG.error(e);
            }
        }

        return null;
    }

    /**
     *
     * @return the list of the current reachable peers
     */
    public synchronized List<String> getPeerList()
    {
        checkConcurrentModification("getPeerList");
        DSProProxyNotInitializedException.check(_commHelper);

        List<String> peerList = null;
        try {
            _commHelper.sendLine("getPeerList");
            _commHelper.receiveMatch("OK");

            int nPeers = _commHelper.read32();
            if (nPeers > 0) {
                peerList = new LinkedList<String>();
                for (int i = 0; i < nPeers; i++) {
                    int idLen = _commHelper.read32();
                    if (idLen > 0) {
                        byte[] buf = new byte[idLen];
                        _commHelper.receiveBlob(buf, 0, idLen);
                        peerList.add(new String(buf));
                    }
                }
                return peerList;
            }
            return null;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.error(e);
        }
        finally {
            return peerList;
        }
    }

    /**
     * 
     * @param peerNodeId
     * @return the node context of the peer identified by peerNodeId
     * @throws us.ihmc.comm.CommException
     */
    public synchronized PeerNodeContext getPeerNodeContext (String peerNodeId) throws CommException
    {
        checkConcurrentModification("getPeerNodeContext");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("getPeerNodeContext");
            _commHelper.sendBlock(peerNodeId.getBytes());
            String hasNodeContextMsg = _commHelper.receiveLine();

            PeerNodeContext peerNodeContext;
            if ("OK".equals(hasNodeContextMsg)) {
                peerNodeContext = PeerNodeContext.read(_commHelper);
            }
            else {
                peerNodeContext = null;
            }
            return peerNodeContext;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return null;
        }
    }

    public synchronized int registerDSProProxyListener (DSProProxyListener listener)
            throws CommException
    {
        checkConcurrentModification("registerDSProxyListener");
        DSProProxyNotInitializedException.check(_commHelper);

        if (_listenerPro == null) {
            LOG.warn("Error:DSProProxyListener is null");
            return -1;
        }

        LOG.info ("registerDSProProxyListener: number of listeners: " + _listenerPro.size());
        if (_listenerPro.isEmpty()) {
            _listenerPro.add(listener);
            try {
                _commHelper.sendLine("registerPathRegisteredCallback");
            }
            catch (Exception e) {
                LOG.warn(StringUtil.getStackTraceAsString(e));
                throw new CommException(e);
            }
        }
        else {
            _listenerPro.add(listener);
        }
        return _listenerPro.size() - 1;
    }

    public synchronized void deregisterDSProProxyListener (DSProProxyListener listener)
            throws CommException
    {
        checkConcurrentModification ("deregisterDSProListener");
        DSProProxyNotInitializedException.check(_commHelper);

        if (_listenerPro == null) {
            LOG.warn ("Error:deregisterDSProListener is null");
            return;
        }

        LOG.info ("deregisterDSProProxyListener: number of listeners: " + _listenerPro.size());
        if (!_listenerPro.isEmpty()) {
            _listenerPro.remove (listener);
            LOG.info ("deregisterDSProProxyListener: number of listeners after remove: " + _listenerPro.size());
            if (_listenerPro.isEmpty()) {
                try {
                    _commHelper.sendLine ("deregisterDSProListener");
                }
                catch (CommException e) {
                    LOG.error ("DSPro is disconnected. Send line for deregisterDSProListener failed", e);
                    throw e;
                }
            }
        }
    }

    public synchronized int registerSearchListener (SearchListener listener)
            throws CommException
    {
        checkConcurrentModification("registerSearchListener");
        DSProProxyNotInitializedException.check(_commHelper);

        if (_searchMsgListeners == null) {
            LOG.warn("Error:_searchMsgListeners is null");
            return -1;
        }

        LOG.info ("registerSearchListener: number of listeners: " + _searchMsgListeners.size());
        if (_searchMsgListeners.isEmpty()) {
            _searchMsgListeners.add(listener);
            try {
                _commHelper.sendLine("registerSearchCallback");
            }
            catch (Exception e) {
                LOG.warn(StringUtil.getStackTraceAsString(e));
                throw new CommException(e);
            }
        }
        else {
            _searchMsgListeners.add(listener);
        }
        return _searchMsgListeners.size() - 1;
    }

    public synchronized void deregisterSearchListener (SearchListener listener)
            throws CommException
    {
        checkConcurrentModification("deregisterSearchListener");
        DSProProxyNotInitializedException.check(_commHelper);

        if (_searchMsgListeners == null) {
            LOG.warn("Error:_searchMsgListeners is null");
            return;
        }

        LOG.info ("deregisterSearchListener: number of listeners: " + _searchMsgListeners.size());
        if (!_searchMsgListeners.isEmpty()) {           
            _searchMsgListeners.remove (listener);
            LOG.info ("deregisterSearchListener: number of listeners after remove: " + _searchMsgListeners.size());
            if (_searchMsgListeners.isEmpty()) {
                try {
                    _commHelper.sendLine ("deregisterSearchCallback");
                }
                catch (CommException e) {
                    LOG.error ("DSPro is disconnected. Send line for deregisterSearchCallback failed", e);
                    throw e;
                }
            }
        }
    }

    public synchronized void reinitialize()
    {
        new Thread()
        {
            @Override
            public void run()
            {
                while (!isInitialized()) {
                    init();
                    try {
                        Thread.sleep (_reinitializationAttemptInterval);
                    }
                    catch (InterruptedException ex) {}
                }
            }
        }.start();
    }

    /**
     * When the behavior of a node is to be connected for some time,
     * then disconnected for some time and then connected again and so on,
     * the application may wish to reset the transmission counters upon
     * reconnection so the communication won't suffer from the period of
     * unreachability.
     * This feature is currently only supported for mockets adaptors.
     * @throws us.ihmc.comm.CommException
     */
    public synchronized void resetTransmissionCounters()
            throws CommException
    {
        checkConcurrentModification("resetTransmissionCounters");
        DSProProxyNotInitializedException.check(_commHelper);

        try {
            _commHelper.sendLine("resetTransmissionCounters");
            _commHelper.receiveMatch("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
        }
    }

    //--------------------------------------------------------------------------
    // Listener Registration
    //--------------------------------------------------------------------------

    public synchronized void registerCtrlMsgListener (CtrlMsgListener listener)
            throws CommException
    {
        checkConcurrentModification("registerCtrlMsgListener");
        DSProProxyNotInitializedException.check(_commHelper);

        if (listener == null) {
            LOG.warn("Error:registerCtrlMsgListener is null");
            return;
        }
        if (_ctrlMsgListeners.isEmpty()) {
            _ctrlMsgListeners.add(listener);
            try {
                _commHelper.sendLine("registerCtrlMsgCallback");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set(false);
                    throw (CommException) e;
                }
                LOG.warn(StringUtil.getStackTraceAsString(e));
            }
        }
        else {
            _ctrlMsgListeners.add(listener);
        }
    }

    public synchronized int registerMatchmakingLogListener (MatchmakingLogListener listener)
            throws CommException
    {
        checkConcurrentModification("registerMatchmakingLogListener");
        DSProProxyNotInitializedException.check(_commHelper);

        if (listener == null) {
            LOG.warn("Error:MatchmakingLogListener is null");
            return -1;
        }
        if (_matchmakerListeners.isEmpty()) {
            _matchmakerListeners.add(listener);
            try {
                _commHelper.sendLine("registerMatchmakingLogCallback");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set(false);
                    throw (CommException) e;
                }
                LOG.warn(StringUtil.getStackTraceAsString(e));
            }
        }
        else {
            _matchmakerListeners.add(listener);
        }
        return _matchmakerListeners.size() - 1;
    }

    public synchronized void deregisterMatchmakingLogListener (int clientId, MatchmakingLogListener listener)
            throws CommException
    {
        checkConcurrentModification("deregisterMatchmakingLogListener");
        DSProProxyNotInitializedException.check(_commHelper);

        if (_matchmakerListeners == null) {
            LOG.warn("Error:deregisterMatchmakingLogListener is null");
            return;
        }
        if (!_matchmakerListeners.isEmpty()) {
            _matchmakerListeners.remove (listener);
            LOG.info ("deregisterSearchListener: number of listeners after remove: " + _searchMsgListeners.size());
            if (_searchMsgListeners.isEmpty()) {
                try {
                    _commHelper.sendLine ("deregisterMatchmakingLogListener");
                }
                catch (CommException e) {
                    LOG.error ("DSPro is disconnected. Send line for deregisterSearchCallback failed", e);
                    throw e;
                }
            }
        }
    }

    public synchronized void registerConnectionStatusListener (ConnectionStatusListener listener)
    {
        checkConcurrentModification("registerConnectionStatusListener");
        DSProProxyNotInitializedException.check(_commHelper);

        if (listener == null) {
            LOG.warn("Error: ConnectionStatusListener is null");
            return;
        }

        _connectionStatusListeners.add(listener);
    }

    //--------------------------------------------------------------------------
    // Callbacks
    //--------------------------------------------------------------------------

    public boolean dataArrived (String id, String groupName, String objectId,
                                String instanceId, String annotatedObjMsgId, String mimeType,
                                byte[] data, short chunkNumber, short totChunksNumber,
                                String queryId)

    {
        return dataArrived(_listenerPro, id, groupName, objectId, instanceId,
                           annotatedObjMsgId, mimeType, data, chunkNumber,
                           totChunksNumber, queryId);
    }

    public static boolean dataArrived (Collection<DSProProxyListener> listeners,
                                       String id, String groupName, String objectId,
                                       String instanceId, String annotatedObjMsgId, String mimeType,
                                       byte[] data, short chunkNumber, short totChunksNumber, String queryId)

    {
        if (id == null || data == null || data.length == 0) {
            throw new NullPointerException("Null paramenters");
        }
        synchronized (listeners) {
            if (listeners.isEmpty()) {
                LOG.warn("DSProProxy::dataArrived: error:DisseminationServiceProxyListener is null");
                return false;
            }

            boolean returnValue = false;
            for (DSProProxyListener listener : listeners) {
                if (listener.dataArrived(id, groupName, objectId, instanceId,
                                         annotatedObjMsgId, mimeType, data,
                                         chunkNumber, totChunksNumber, queryId)) {
                    returnValue = true;
                }
            }

            return returnValue;
        }
    }

    public boolean metadataArrived (String id, String groupName, String objectId, String instanceId,
                                    String XMLMetadata, String referredDataId, String queryId)
    {
        return metadataArrived(_listenerPro, id, groupName, objectId, instanceId, XMLMetadata, referredDataId, queryId);
    }

    public static boolean metadataArrived (Collection<DSProProxyListener> listeners,
                                           String id, String groupName, String objectId,
                                           String instanceId, String XMLMetadata,
                                           String referredDataId, String queryId)
    {
        if (id == null || XMLMetadata == null || XMLMetadata.length() == 0 || referredDataId == null) {
            throw new NullPointerException("Null paramenters");
        }

        System.out.println("DSProProxy: metadataArrived: " + id + " about to notify listeners: before synchronized");

        synchronized (listeners) {
            if (listeners.isEmpty()) {
                LOG.warn("NOMADS:DSProProxy::metadataArrived: error:DisseminationServiceProxyListener is null");
                return false;
            }

            System.out.println("NOMADS:DSProProxy: metadataArrived: " + id + " about to notify listeners");

            boolean returnValue = false;
            for (DSProProxyListener listener : listeners) {
                if (listener.metadataArrived(id, groupName, objectId, instanceId,
                        XMLMetadata, referredDataId, queryId)) {
                    returnValue = true;
                }
            }

            System.out.println("NOMADS:DSProProxy: metadataArrived: " + id + " notified listeners");

            return returnValue;
        }
    }

    public void newNeighbor (String peerID)
    {
        newNeighbor(_listenerPro, peerID);
    }

    public static void newNeighbor (Collection<DSProProxyListener> listeners, String peerID)
    {
        if (listeners.isEmpty()) {
            LOG.warn("DSProProxy::newNeighbor: error:DisseminationServiceProxyListener is null");
            return;
        }

        for (DSProProxyListener listener : listeners) {
            listener.newNeighbor(peerID);
        }
    }

    public void deadNeighbor (String peerID)
    {
        deadNeighbor(_listenerPro, peerID);
    }

    public static void deadNeighbor (Collection<DSProProxyListener> listeners, String peerID)
    {
        if (listeners.isEmpty()) {
            LOG.warn("DSProProxy::deadNeighbor: error:DisseminationServiceProxyListener is null");
            return;
        }

        for (DSProProxyListener listener : listeners) {
            listener.deadNeighbor(peerID);
        }
    }

    public boolean pathRegistered (NodePath path, String nodeId, String team, String mission)
    {
        return pathRegistered(_listenerPro, path, nodeId, team, mission);
    }

    public static boolean pathRegistered (Collection<DSProProxyListener> listeners, NodePath path, String nodeId,
                                          String team, String mission)
    {
        synchronized (listeners) {
            if (listeners.isEmpty()) {
                LOG.warn("DSProProxy::pathRegistered: error:DisseminationServiceProxyListener is null");
                return false;
            }

            boolean returnValue = false;
            for (DSProProxyListener listener : listeners) {
                if (listener.pathRegistered(path, nodeId, team, mission)) {
                    returnValue = true;
                }
            }

            return returnValue;
        }
    }

    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId)
    {
        return positionUpdated (_listenerPro, latitude, longitude, altitude, nodeId);
    }

    public static boolean positionUpdated (Collection<DSProProxyListener> listeners, float latitude, float longitude,
                                           float altitude, String nodeId)
    {
        synchronized (listeners) {
            if (listeners.isEmpty()) {
                LOG.warn("DSProProxy::positionUpdated: error:DisseminationServiceProxyListener is null");
                return false;
            }

            boolean returnValue = false;
            for (DSProProxyListener listener : listeners) {
                if (listener.positionUpdated(latitude, longitude, altitude, nodeId)) {
                    returnValue = true;
                }
            }

            return returnValue;
        }
    }

    public void searchArrived (String queryId, String groupName, String querier,
                                  String queryType, String queryQualifiers, byte[] query)
    {
        searchArrived (_searchMsgListeners, queryId, groupName, querier, queryType, queryQualifiers, query);
    }

    public static void searchArrived (Collection<SearchListener> listeners, String queryId, String groupName,
                                         String querier, String queryType, String queryQualifiers, byte[] query)
    {
        if (listeners.isEmpty()) {
            LOG.warn("DSProProxy::searchArrived: error:DisseminationServiceProxyListener is null");
        }

        for (SearchListener listener : listeners) {
            listener.searchArrived (queryId, groupName, querier, queryType, queryQualifiers, query);
        }
    }

    public void searchReplyArrived (String queryId, Collection<String> matchingIds,
                                    String responderNodeId)
    {
        searchReplyArrived (_searchMsgListeners, queryId, matchingIds, responderNodeId);
    }

    public static void searchReplyArrived (Collection<SearchListener> listeners, String queryId,
                                           Collection<String> matchingIds, String responderNodeId)
    {
        if (listeners.isEmpty()) {
            LOG.warn("DSProProxy::searchReplyArrived: error:DisseminationServiceProxyListener is null");
        }

        for (SearchListener listener : listeners) {
            listener.searchReplyArrived (queryId, matchingIds, responderNodeId);
        }
    }

    public void searchReplyArrived (String queryId, byte[] reply, String responderNodeId)
    {
        searchReplyArrived (_searchMsgListeners, queryId, reply, responderNodeId);
    }

    public static void searchReplyArrived (Collection<SearchListener> listeners, String queryId,
                                           byte[] reply, String responderNodeId)
    {
        if (listeners.isEmpty()) {
            LOG.warn("DSProProxy::searchReplyArrived: error:DisseminationServiceProxyListener is null");
        }

        for (SearchListener listener : listeners) {
            listener.searchReplyArrived (queryId, reply, responderNodeId);
        }
    }

    public void informationMatched (String localNodeID, String peerNodeID,
                                    String skippedObjectID, String skippedObjectName,
                                    String[] rankDescriptors,
                                    float[] partialRanks, float[] weights,
                                    String comment, String operation)
    {
        synchronized (_matchmakerListeners) {
            if (_matchmakerListeners.isEmpty()) {
                LOG.warn("DSProProxy::informationMatched: error:DisseminationServiceProxyListener is null");

            }

            for (MatchmakingLogListener _matchmakerListener : _matchmakerListeners) {
                _matchmakerListener.informationMatched(localNodeID, peerNodeID,
                        skippedObjectID, skippedObjectName, rankDescriptors,
                        partialRanks, weights, comment, operation);
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
                LOG.warn("DSProProxy::informationSkipped: error:DisseminationServiceProxyListener is null");
            }

            for (MatchmakingLogListener _matchmakerListener : _matchmakerListeners) {
                _matchmakerListener.informationSkipped(localNodeID, peerNodeID,
                        skippedObjectID, skippedObjectName, rankDescriptors,
                        partialRanks, weights, comment, operation);
            }
        }
    }

    public void notifyConnectionLoss()
    {
        _isInitialized.set(false);
        for (ConnectionStatusListener _connectionStatusListener : _connectionStatusListeners) {
            _connectionStatusListener.connectionLost();
        }
    }

    private CommHelper connectToServer (String host, int port)
            throws Exception
    {
        CommHelper commHelper = new CommHelper();
        Socket socket = new Socket(host, port);
        // The DSPro proxy communication is fairly interactivity, so
        // it is better to disable the Nagle's algorithm
        socket.setTcpNoDelay(true);
        commHelper.init (socket);
        return commHelper;
    }

    private void doHandshake (CommHelper ch)
         throws CommException, ProtocolException
    {
        String handshake = new StringBuilder(PROTOCOL).append(" ").append(VERSION).toString();
        System.out.println("Sending handshake:" + handshake);
        ch.sendLine (handshake);
        String[] tokens = ch.receiveParsed();
        if (tokens == null) {
            throw new ProtocolException ("could not complete handshake: did not receive service and version");
        }
        if (tokens.length != 2) {
            throw new ProtocolException ("could not complete handshake: did not receive service and version");
        }
        if ((PROTOCOL.compareToIgnoreCase (tokens[0]) != 0) || (VERSION.compareToIgnoreCase (tokens[1]) != 0)) {
            throw new ProtocolException ("could not complete handshake: received the following " + tokens[0] + " " + tokens[1]
                    + " service and version while expecting " + PROTOCOL + " and " + VERSION);
        }
        System.out.println("Handshake successful");
    }

    private int registerProxy (CommHelper ch, CommHelper chCallback, int desiredApplicationId) throws CommException
    {
        int applicationId = 0;
        try {
            // First register the proxy using the desired application id
            // The ProxyServer will return the assigned application id
            ch.sendLine("RegisterProxy " + desiredApplicationId);
            String[] array = ch.receiveRemainingParsed("OK");
            applicationId = Short.parseShort(array[0]);

            // Now register the callback using the assigned application id
            chCallback.sendLine("RegisterProxyCallback " + applicationId);
            chCallback.receiveMatch("OK");
            return applicationId;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set(false);
                throw (CommException) e;
            }
            LOG.warn(StringUtil.getStackTraceAsString(e));
            return -1;
        }
    }

    public void contextUpdateMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.contextUpdateMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void contextVersionMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.contextVersionMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void messageRequestMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.messageRequestMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void chunkRequestMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.chunkRequestMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void positionMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.positionMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void searchMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.searchMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void topologyReplyMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.topologyReplyMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void topologyRequestMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.topologyRequestMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void updateMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.updateMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void versionMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.versionMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void waypointMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.waypointMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public void wholeMessageArrived (String senderNodeId, String publisherNodeId)
    {
        for (CtrlMsgListener listener : _ctrlMsgListeners) {
            listener.wholeMessageArrived(senderNodeId, publisherNodeId);
        }
    }

    public class DataWrapper
    {
        public DataWrapper (byte[] data, boolean hasMoreChunks, String queryId)
        {
            _data = data;
            _hasMoreChunks = hasMoreChunks;
            _queryId = queryId;
        }

        public final byte[] _data;
        public final boolean _hasMoreChunks;
        public final String _queryId;
    }

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

    public static final int DIS_SVC_PROXY_SERVER_PORT_NUMBER = 56487;

    // Constant for to Path type
    public static final String _PAST_PATH = "Node Past Path";
    public static final int _MAIN_PATH_TO_OBJECTIVE = 1;
    public static final int _ALTERNATIVE_PATH_TO_OBJECTIVE = 2;
    public static final int _MAIN_PATH_TO_BASE = 3;
    public static final int _ALTERNATIVE_PATH_TO_BASE = 4;
    public static final int _FIXED_LOCATION = 5;

    protected String _nodeId;
    protected DSProProxyCallbackHandler _handler;
    protected final AtomicBoolean _isInitialized = new AtomicBoolean(false);
    protected final ArrayList<ConnectionStatusListener> _connectionStatusListeners = new
            ArrayList<ConnectionStatusListener>();

    private short _applicationId;
    private int _port;
    private String _host;
    private long _reinitializationAttemptInterval;
    private CommHelper _commHelper;
    private final List<DSProProxyListener> _listenerPro = new ArrayList<DSProProxyListener>();
    private final List<MatchmakingLogListener> _matchmakerListeners = new ArrayList<MatchmakingLogListener>();
    private final List<CtrlMsgListener> _ctrlMsgListeners = new ArrayList<CtrlMsgListener>();
    private final List<SearchListener> _searchMsgListeners = new ArrayList<SearchListener>();
    private final static LoggerInterface LOG = LoggerWrapper.getLogger(DSProProxy.class);
}
