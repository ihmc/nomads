/*
 * DisseminationServiceProxy.java
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.aci.disServiceProxy;

import java.net.Socket;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.StringUtil;

/**
 * Dissemination Service Proxy
 *
 * Date: May 30, 2008
 * Time: 3:24:57 PM
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision: 1.93 $
 *
 */
public class DisseminationServiceProxy implements DisseminationServiceInterface
{
    public DisseminationServiceProxy ()
    {
        this (new CallbackHandlerFactory(), (short)0);
        _reinitializationAttemptInterval = 5000;
    }

    /**
     * 
     * @param applicationId 
     */
    public DisseminationServiceProxy (short applicationId)
    {
        this (new CallbackHandlerFactory(), applicationId, DEFAULT_HOST, DIS_SVC_PROXY_SERVER_PORT_NUMBER);
         _reinitializationAttemptInterval = 5000;
    }

    public DisseminationServiceProxy (short applicationId, long reinitializationAttemptInterval)
    {
        this (new CallbackHandlerFactory(), applicationId);
        _reinitializationAttemptInterval = reinitializationAttemptInterval;
    }

    /**
     *
     * @param applicationId - an identifier that identify the application.
     * @param host - the host where the Dissemination Service Proxy is running.
     * @param port - the port on which the Dissemination Service Proxy is listening.
     */
    public DisseminationServiceProxy (short applicationId, String host, int port)
    {
        _applicationId = applicationId;
        _host = host;
        _port = port;
        _nodeId = null;
        _reinitializationAttemptInterval = 5000;
        _handlerFactory = new CallbackHandlerFactory();
    }

    public DisseminationServiceProxy (short applicationId, String host, int port,
                                      long reinitializationAttemptInterval)
    {
        this (applicationId, host, port);
        _reinitializationAttemptInterval = reinitializationAttemptInterval;
    }

    // Protected constructors: they are a copy of the public constructors but can
    // be used by the subclass to use it's own CallbackHandlerFactory
    protected DisseminationServiceProxy (CallbackHandlerFactory handlerFactory, short applicationId,
                                         String host, int port)
    {
        _applicationId = applicationId;
        _host = host;
        _port = port;
        _nodeId = null;
        _reinitializationAttemptInterval = 5000;
        _handlerFactory = handlerFactory;
    }

    protected DisseminationServiceProxy (CallbackHandlerFactory handlerFactory, short applicationId,
                                         long reinitializationAttemptInterval)
    {
        this (handlerFactory, applicationId);
        _reinitializationAttemptInterval = reinitializationAttemptInterval;
    }

    protected DisseminationServiceProxy (CallbackHandlerFactory handlerFactory, short applicationId)
    {
        this (handlerFactory, applicationId, DEFAULT_HOST, DIS_SVC_PROXY_SERVER_PORT_NUMBER);
         _reinitializationAttemptInterval = 5000;
    }

    protected DisseminationServiceProxy (CallbackHandlerFactory handlerFactory)
    {
        this (handlerFactory, (short)0);
        _reinitializationAttemptInterval = 5000;
    }

    protected DisseminationServiceProxy (CallbackHandlerFactory handlerFactory, short applicationId,
                                         String host, int port, long reinitializationAttemptInterval)
    {
        this (applicationId, host, port);
        _reinitializationAttemptInterval = reinitializationAttemptInterval;
    }

    public int init() throws Exception
    {
        int rc = 0;
        if (_host == null) {
            _host = "127.0.0.1";
        }
        if (_port == 0) {
            _port = DIS_SVC_PROXY_SERVER_PORT_NUMBER;
        }

        try {
            CommHelper ch = connectToServer (_host, _port);
            CommHelper chCallback = connectToServer (_host, _port);
            if (ch != null && chCallback != null) {
                int applicationId = registerProxy (ch, chCallback, _applicationId);
                if (applicationId >= 0) {
                    _applicationId = (short)applicationId; // The server may have assigned
                                                           // a different id than requested
                    _commHelper = ch;
                    _handler = _handlerFactory.getHandler (this, chCallback);
                    _handler.start();
                    _isInitialized.set (true);
                }
                else {
                    rc = -1;
                }
            }

            _nodeId = getNodeId();
        }
        catch (Exception e) {
            rc = -1;
            throw e;
        }

        if (rc < 0) {
            throw new Exception ("DisseminationServiceProxy:init:Failed to register listener.");
        }
        return rc;
    }

    public void reinitialize()
    {
        new Thread(){
            @Override
            public void run() {
                while (!isInitialized()) {
                    try {
                        init();
                        Thread.sleep (_reinitializationAttemptInterval);
                    }
                    catch (InterruptedException ex) {}
                    catch (Exception ex) {
                        Logger.getLogger(DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, ex);
                    }                    
                }
            }
        }.start();
    }

    public boolean isInitialized()
    {
        return _isInitialized.get();
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
        if (objectId == null) {
            return null;
        }

        checkConcurrentModification("getDisServiceId");

        try {
            _commHelper.sendLine("getDisServiceId");
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
            Logger.getLogger(DisseminationServiceProxy.class.getName()).log(Level.SEVERE, StringUtil.getStackTraceAsString(e), e);
            return null;
        }
    }

    public String getNodeId() throws CommException
    {
        checkConcurrentModification ("getNodeId");

        if (_nodeId != null) {
            return _nodeId;
        }

        try {
            _commHelper.sendLine ("getNodeId");
            _commHelper.receiveMatch ("OK");

            return _commHelper.receiveLine();
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, StringUtil.getStackTraceAsString(e), e);
        }

	return null;
    }

    public String getSessionId() throws CommException
    {
        return null;
    }

    public List<String> getPeerList()
    {
        checkConcurrentModification ("getPeerList");

        List<String> peerList = null;
        try {
            _commHelper.sendLine ("getPeerList");
            _commHelper.receiveMatch ("OK");

            int nPeers = _commHelper.read32();
            if (nPeers > 0) {
                peerList = new LinkedList<String>();
                for (int i = 0; i < nPeers; i++) {
                    int idLen = _commHelper.read32();
                    if (idLen > 0) {
                        byte[] buf = new byte[idLen];
                        _commHelper.receiveBlob(buf, 0, idLen);
                        peerList.add(new String (buf));
                    }
                }
                return peerList;
            }
            return null;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }
        finally {
            return peerList;
        }
    }

    /**
     * Caches a data within a group/tag.
     * The data is not disseminated over the network at this time.
     *
     * @param groupName - the group to which the message was addressed.
     * @param applicationId
     * @param instanceId
     * @param mimeType
     * @param metaData - information about the content of the message. Format
     *                   and content of this parameter are opaque to the
     *                   Dissemination Service. The metadata can be null.
     * @param data - the data to be published.
     * @param expiration - the length of time (in milliseconds) after which the
     *                     data becomes obsolete. A value of 0 means that the
     *                     data has no expiration.
     * @param historyWindow - the number of previous messages that are suggested
     *                        to be retrieved in order to fully make sense of the
     *                        message being published
     * @param tag - the tag that was specified for the message
     * @param priority- the priority value that was specified for the message.
     * @return 
     * @throws us.ihmc.comm.CommException 
     */
    public synchronized String store (String groupName, String applicationId, String instanceId,
                                      String mimeType, byte[] metaData, byte[] data, long expiration,
                                      short historyWindow, short tag, byte priority)
        throws CommException
    {
        checkConcurrentModification ("store");

        String msgId = null;
        try {
            _commHelper.sendLine ("store");
            _commHelper.sendLine (groupName);
            _commHelper.sendStringBlock (applicationId);
            _commHelper.sendStringBlock (instanceId);
            _commHelper.sendStringBlock (mimeType);
            if (metaData == null) {
                _commHelper.write32 (0);
            }
            else {
                _commHelper.write32 (metaData.length);
                _commHelper.sendBlob (metaData);
            }
            _commHelper.write32 (data.length);
            _commHelper.sendBlob (data);
            _commHelper.write64 (expiration);
            _commHelper.write16 (historyWindow);
            _commHelper.write16 (tag);
            _commHelper.write8 (priority);

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }

        //read the ID
        try {
            msgId = _commHelper.receiveLine();
        }
         catch (Exception e) {
            System.out.println ("Exception: " + e);
        }

        return msgId;
    }

    public synchronized void push (String msgId)
        throws CommException
    {
        if (msgId == null || msgId.length() == 0) {
            return;
        }

        checkConcurrentModification ("pushById");

        try {
            _commHelper.sendLine ("pushById");
            _commHelper.sendBlock (msgId.getBytes());

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }
    }

    /**
     * Publishes a data within a group/tag.
     * The data is immediately disseminated over the network.
     *
     * @param groupName - the group to which the message was addressed.
     * @param objectId
     * @param instanceId
     * @param mimeType
     * @param metaData - information about the content of the message. Format
     *                   and content of this parameter are opaque to the
     *                   Dissemination Service. The metadata can be null.
     * @param data - the data to be published.
     * @param expiration - the length of time (in milliseconds) after which the
     *                     data becomes obsolete. A value of 0 means that the
     *                     data has no expiration.
     * @param historyWindow - the number of previous messages that are suggested
     *                        to be retrieved in order to fully make sense of the
     *                        message being published
     * @param tag - the tag that was specified for the message
     * @param priority- the priority value that was specified for the message.
     * @return 
     * @throws us.ihmc.comm.CommException 
     */
    public synchronized String push (String groupName, String objectId, String instanceId, String mimeType,
                                     byte[] metaData, byte[] data, long expiration, short historyWindow,
                                     short tag, byte priority)
        throws CommException
    {
        checkConcurrentModification ("push");

        String msgId = null;
        try {
            _commHelper.sendLine ("push");
            _commHelper.sendLine (groupName);
            _commHelper.sendStringBlock (objectId);
            _commHelper.sendStringBlock (instanceId);
            _commHelper.sendStringBlock (mimeType);
            if (metaData == null) {
                _commHelper.write32 (0);
            }
            else {
                _commHelper.write32 (metaData.length);
                _commHelper.sendBlob (metaData);
            }
            _commHelper.write32 (data.length);
            _commHelper.sendBlob (data);
            _commHelper.write64 (expiration);
            _commHelper.write16 (historyWindow);
            _commHelper.write16 (tag);
            _commHelper.write8 (priority);

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }

        //read the ID
        try {
            msgId = _commHelper.receiveLine();
        }
         catch (Exception e) {
            System.out.println ("Exception: " + e);
        }

        return msgId;
    }

    /**
     * Publishes a data within a group/tag.
     * The metadata part is immediately disseminated over the network, the data
     * is sent only upon request.
     *
     * @param groupName - the group to which the message was addressed.
     * @param objectId
     * @param instanceId
     * @param metadata - information about the content of the message. Format
     *                   and content of this parameter are opaque to the
     *                   Dissemination Service. The metadata can not be null.
     * @param data - the data to be published.
     * @param dataMimeType - the MIME type of the data being published. If null
     *                       
     * @param expiration - the length of time (in milliseconds) after which the
     *                     data becomes obsolete. A value of 0 means that the
     *                     data has no expiration.
     * @param historyWindow - the number of previous messages that are suggested
     *                        to be retrieved in order to fully make sense of the
     *                        message being published
     * @param tag - the tag that was specified for the message
     * @param priority- the priority value that was specified for the message.
     * @return 
     * @throws us.ihmc.comm.CommException 
     */
    public synchronized String makeAvailable (String groupName, String objectId, String instanceId, byte[] metadata,
                                              byte[] data, String dataMimeType, long expiration, short historyWindow,
                                              short tag, byte priority)
        throws CommException
    {
        checkConcurrentModification ("makeAvailable");

        String msgId = null;
        try {
            _commHelper.sendLine ("makeAvailable");
            _commHelper.sendLine (groupName);
            _commHelper.sendStringBlock (objectId);
            _commHelper.sendStringBlock (instanceId);
            if (metadata == null) {
                _commHelper.write32 (0);
            }
            else {
                _commHelper.write32 (metadata.length);
                _commHelper.sendBlob (metadata);
            }
            _commHelper.write32 (data.length);
            _commHelper.sendBlob (data);
            _commHelper.sendStringBlock (dataMimeType);
            _commHelper.write64 (expiration);
            _commHelper.write16 (historyWindow);
            _commHelper.write16 (tag);
            _commHelper.write8 (priority);

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }

        //read the ID
        try {
            msgId = _commHelper.receiveLine();
        }
        catch (Exception e) {
            System.out.println ("Exception: " + e);
        }

        return msgId;
    }

    public synchronized void cancel (String id)
	    throws CommException, ProtocolException
    {
        checkConcurrentModification ("cancel - 0");

        try {
            _commHelper.sendLine ("cancel_str");
            _commHelper.sendLine (id);

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            if (e instanceof ProtocolException) {
                throw (ProtocolException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }
    }

    public synchronized void cancel (short tag)
	    throws CommException, ProtocolException
    {
        checkConcurrentModification ("cancel - 1");

        try {
            _commHelper.sendLine ("cancel_str");
            _commHelper.write16 (tag);

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            if (e instanceof ProtocolException) {
                throw (ProtocolException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }
    }

    public synchronized boolean addFilter (String groupName, short tag)
        throws CommException
    {
        checkConcurrentModification ("addFilter");

        try {
            _commHelper.sendLine ("addFilter");
            _commHelper.sendLine (groupName);
            _commHelper.write16 (tag);

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }

        return true;
    }

    public synchronized boolean removeFilter (String groupName, short tag)
        throws CommException
    {
        checkConcurrentModification ("removeFilter");

        try {
            _commHelper.sendLine ("removeFilter");
            _commHelper.sendLine (groupName);
            _commHelper.write16 (tag);

            _commHelper.receiveMatch ("OK");             
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }

        return true;
    }

    public synchronized boolean requestMoreChunks (String groupName, String senderNodeId, int seqId)
        throws CommException
    {
        checkConcurrentModification ("requestMoreChunks - 1");

        try {
            _commHelper.sendLine ("requestMoreChunks");
            _commHelper.sendLine (groupName);
            _commHelper.sendLine (senderNodeId);
	    _commHelper.write32 (seqId);

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }

        return true;
    }

    public synchronized boolean requestMoreChunks (String messageId)
        throws CommException
    {
        checkConcurrentModification ("requestMoreChunks - 2");

        try {
            _commHelper.sendLine ("requestMoreChunksByID");
            _commHelper.sendLine (messageId);

            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }

        return true;
    }

    public synchronized byte[] retrieve (String id, int timeout)
        throws CommException
    {
        checkConcurrentModification ("retrieve - 0");

        byte[] buff = null;
        try {
            _commHelper.sendLine ("retrieve");
            _commHelper.sendLine (id);
            //_commHelper.write32 (buff.length);
            _commHelper.write64 (timeout);

            // Read the returned object
            _commHelper.receiveMatch ("OK");
            int bufLen = _commHelper.read32();
            if (bufLen > 0) {
                buff = new byte [bufLen];
                _commHelper.receiveBlob (buff, 0, bufLen);
            }
            _commHelper.receiveMatch ("OK");
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            buff = null;
        }

        return buff;
    }

    public synchronized int retrieve (String id, String filePath)
    {
        //Not implemented yet
        return 0;
    }

    public synchronized boolean request (String groupName, short tag, short historyLength, long timeout)
        throws CommException
    {
        checkConcurrentModification ("request");

        try {
            _commHelper.sendLine ("request");
            _commHelper.sendLine (groupName);
            _commHelper.write16 (tag);
            _commHelper.write16 (historyLength);
            _commHelper.write64 (timeout);

            _commHelper.receiveMatch ("OK");

            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            return false;
        }
    }

    public synchronized boolean request (String msgId, long timeout)
        throws CommException
    {
        checkConcurrentModification ("request_id");

        try {
            _commHelper.sendLine ("request_id");
            _commHelper.sendLine (msgId);
            _commHelper.write64 (timeout);

            _commHelper.receiveMatch ("OK");

            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            return false;
        }
    }

    public synchronized String search (String groupName, String queryType,
                                       String queryQualifiers, byte[] query)
        throws CommException
    {
        checkConcurrentModification("search");

        if (groupName == null || query == null) {
            throw new NullPointerException();
        }

    	try {
            _commHelper.sendLine ("search");
            _commHelper.sendStringBlock (groupName);
            _commHelper.sendStringBlock (queryType);
            _commHelper.sendStringBlock (queryQualifiers);
            _commHelper.write32 (query.length);
            _commHelper.sendBlob (query);

            _commHelper.receiveMatch ("OK");

            // read the ID assigned to the query
            byte[] b = _commHelper.receiveBlock();
            String queryId = b != null ? new String (b) : null;
 
            return queryId;
    	}
    	catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
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
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
        }
    }

    public synchronized boolean subscribe (String groupName, byte priority, boolean groupReliable,
                                           boolean msgReliable, boolean sequenced)
        throws CommException
    {
        checkConcurrentModification ("subscribe - 0");

        try {
            _commHelper.sendLine ("subscribe");
            _commHelper.sendLine (groupName);
            _commHelper.write8 (priority);
            _commHelper.write8 ((byte) (groupReliable ? 1 : 0));
            _commHelper.write8 ((byte) (msgReliable ? 1 : 0));
            _commHelper.write8 ((byte)(sequenced ? 1 : 0));

            _commHelper.receiveMatch ("OK");
            
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            return false;
        }
    }

    public synchronized boolean subscribe (String groupName, short tag, byte priority,
                                           boolean groupReliable, boolean msgReliable, boolean sequenced)
        throws CommException
    {
        checkConcurrentModification ("subscribe - 1");

        try {
            _commHelper.sendLine ("subscribe_tag");
            _commHelper.sendLine (groupName);
            _commHelper.write8 (priority);
            _commHelper.write16 (tag);
            _commHelper.write8 ((byte) (groupReliable ? 1 : 0));
            _commHelper.write8 ((byte) (msgReliable ? 1 : 0));
            _commHelper.write8 ((byte)(sequenced ? 1 : 0));

            _commHelper.receiveMatch ("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            return false;
        }
    }

    public synchronized boolean subscribe (String groupName, byte predicateType, String predicate, byte priority,
                                           boolean groupReliable, boolean msgReliable, boolean sequenced) throws CommException
    {
        checkConcurrentModification ("subscribe - 2");

        try {
            _commHelper.sendLine ("subscribe_predicate");
            _commHelper.sendLine (groupName);
            _commHelper.write8 (predicateType);
            _commHelper.sendLine (predicate);
            _commHelper.write8 (priority);
            _commHelper.write8 ((byte) (groupReliable ? 1 : 0));
            _commHelper.write8 ((byte) (msgReliable ? 1 : 0));
            _commHelper.write8 ((byte)(sequenced ? 1 : 0));

            _commHelper.receiveMatch ("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            return false;
        }
    }

    public synchronized boolean unsubscribe (String groupName) throws CommException
    {
        checkConcurrentModification ("unscribe - 0");

        try {
            _commHelper.sendLine ("unsubscribe");
            _commHelper.sendLine (groupName);

            _commHelper.receiveMatch ("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            return false;
        }
    }

    public synchronized boolean unsubscribe (String groupName, short tag) throws CommException
    {
        checkConcurrentModification ("unsubscribe - 1");

        try {
            _commHelper.sendLine ("unsubscribe");
            _commHelper.sendLine (groupName);
            _commHelper.write16 (tag);

            _commHelper.receiveMatch ("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            return false;
        }
    }

    @SuppressWarnings("empty-statement")
    public void registerDisseminationServiceProxyListener (DisseminationServiceProxyListener listener) throws CommException
    {
        checkConcurrentModification ("registerDisseminationServiceProxyListener");

        if (listener == null) {
            System.out.println ("Error:DisseminationServiceProxyListener is null");
            return;
        }
        if (_listeners.isEmpty()) {
            _listeners.put((short)0, listener);
            try {
                _commHelper.sendLine ("registerDataArrivedCallback");
            }
            catch (Exception e) {
                if (e instanceof CommException) {
                    _isInitialized.set (false);
                    throw (CommException) e;
                }
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
        else {
            // Search for an unused ID
            short i = 0;
            for (; _listeners.containsKey(i); i++);
            _listeners.put(i, listener);
        }
    }

    public void registerDisseminationServiceProxyListener (short clientId, DisseminationServiceProxyListener listener)
            throws ListenerAlreadyRegisteredException
    {
        checkConcurrentModification ("registerDisseminationServiceProxyListener");

        if (listener == null) {
            System.out.println ("Error:DisseminationServiceProxyListener is null");
            return;
        }
        if(_listeners.isEmpty()) {
            _listeners.put(clientId, listener);
            try {
                _commHelper.sendLine ("registerDataArrivedCallback");
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
        else if (!_listeners.containsKey(clientId)){
            _listeners.put(clientId, listener);
        }
        else {
            throw new ListenerAlreadyRegisteredException ("Listener with ID " + clientId + " has already been registered");
        }
    }

    public void registerPeerStatusListener (PeerStatusListener listener)
    {
        checkConcurrentModification ("registerPeerStatusListener");

        if (listener == null) {
            System.out.println ("Error:PeerStatusListener is null");

            return;
        }
        if(_peerStatusListeners.isEmpty()) {
            _peerStatusListeners.add(listener);
            try {
                _commHelper.sendLine ("registerPeerStatusCallback");
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
        else {
            _peerStatusListeners.add (listener);
        }
    }
	
    public void registerConnectionStatusListener (ConnectionStatusListener listener)
    {
        checkConcurrentModification ("registerConnectionStatusListener");

        if (listener == null) {
            System.out.println ("Error: ConnectionStatusListener is null");
            return;
        }

        _connectionStatusListeners.add (listener);
    }

    public void registerSearchListener (SearchListener listener)
    {
        checkConcurrentModification ("registerSearchListener");

        if (listener == null) {
            System.out.println ("Error:registerSearchListener is null");

            return;
        }
        if( _searchListeners.isEmpty()) {
            _searchListeners.add(listener);
            try {
                _commHelper.sendLine ("registerSearchListener");
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
        else {
            _searchListeners.add (listener);
        }
    }

    public boolean resetTransmissionHistory() throws CommException
    {
        checkConcurrentModification ("resetTransmissionHistory");

        try {
            _commHelper.sendLine ("subscribe_tag");
            _commHelper.receiveMatch ("OK");
            return true;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger(DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            e.printStackTrace();
            return false;
        }
    }

    public void dataArrived (String msgId, String sender, String groupName, int seqNum,
                                String objectId, String instanceId, String mimeType,
                                byte[] data, int metadataLength, short tag,
                                byte priority, String queryId)
    {
        dataArrived (_listeners.values(), msgId, sender, groupName, seqNum, objectId,
                   instanceId, mimeType, data, metadataLength, tag, priority,
                   queryId);
    }

    public static void dataArrived (Collection<? extends DisseminationServiceProxyListener> listeners,
                                   String msgId, String sender, String groupName, int seqNum, String objectId,
                                   String instanceId, String mimeType, byte[] data, int metadataLength, short tag,
                                   byte priority, String queryId)
    {
        if (listeners.isEmpty()) {
            Logger.getLogger (DisseminationServiceProxy.class.getName())
                              .log(Level.INFO, "DisseminationServiceProxy::dataArrived: error:DisseminationServiceProxyListener is null");
            return;
        }

        for (DisseminationServiceProxyListener listener : listeners) {
            try {
                listener.dataArrived (msgId, sender, groupName, seqNum, objectId, instanceId,
                                      mimeType, data, metadataLength, tag, priority, queryId);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    public void chunkArrived (String sender, String msgId, String groupName, int seqNum,
                              String objectId, String instanceId, String mimeType, byte[] data,
                              short nChunks, short totNChunks, String chunkMsgId, short tag,
                              byte priority, String queryId)
    {
        chunkArrived (_listeners.values(), msgId, sender, groupName, seqNum, objectId,
                     instanceId, mimeType, data, nChunks, totNChunks, chunkMsgId,
                     tag, priority, queryId);
    }

    public static void chunkArrived (Collection<? extends DisseminationServiceProxyListener> listeners,
                                     String msgId, String sender, String groupName, int seqNum,
                                     String objectId, String instanceId, String mimeType, byte[] data,
                                     short nChunks, short totNChunks, String chunkMsgId, 
                                     short tag, byte priority, String queryId)
    {
        if (listeners.isEmpty()) {
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.INFO, "DisseminationServiceProxy::chunkArrived: error:DisseminationServiceProxyListener is null");
            return;
        }

        for (DisseminationServiceProxyListener listener : listeners) {
            try {
                listener.chunkArrived (msgId, sender, groupName, seqNum, objectId,
                                       instanceId, mimeType, data, nChunks,
                                       totNChunks, chunkMsgId, tag, priority,
                                       queryId);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    public void metadataArrived (String msgId, String sender, String groupName, int seqNum,
                                    String objectId, String instanceId, String mimeType, byte[] data,
                                    boolean dataChunk, short tag, byte priority, String queryId)
    {
        metadataArrived (_listeners.values(), msgId, sender, groupName, seqNum, objectId,
                       instanceId, mimeType, data, dataChunk, tag, priority, queryId);
    }

    public static void metadataArrived (Collection<? extends DisseminationServiceProxyListener> listeners,
                                        String msgId, String sender, String groupName, int seqNum,
                                        String objectId, String instanceId, String mimeType, byte[] data,
                                        boolean dataChunk, short tag, byte priority, String queryId)
    {
        if (listeners.isEmpty()) {
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.INFO, "DisseminationServiceProxy::metadataArrived: error:DisseminationServiceProxyListener is null");
            return;
        }

        for (DisseminationServiceProxyListener listener : listeners) {
            try {
                listener.metadataArrived (msgId, sender, groupName, seqNum, objectId,
                                          instanceId, mimeType, data, dataChunk,
                                          tag, priority, queryId);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    public void dataAvailable (String msgId, String sender, String groupName, int seqNum,
                                  String objectId, String instanceId, String mimeType, String id,
                                  byte[] data, short tag, byte priority, String queryId)
    {
        dataAvailable (_listeners.values(), msgId, sender, groupName, seqNum, objectId,
                    instanceId, mimeType, id, data, tag, priority, queryId);
    }

    public static void dataAvailable (Collection<? extends DisseminationServiceProxyListener> listeners,
                                         String msgId, String sender, String groupName, int seqNum,
                                         String objectId, String instanceId, String mimeType, String id,
                                         byte[] data, short tag, byte priority, String queryId)
    {
        if (listeners.isEmpty()) {
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.INFO, "DisseminationServiceProxy::dataAvailable: error:DisseminationServiceProxyListener is null");
            return;
        }

        for (DisseminationServiceProxyListener listener : listeners) {
            try {
                listener.dataAvailable (msgId, sender, groupName, seqNum, objectId, instanceId,
                                        mimeType, id, data, tag, priority, queryId);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    public void newPeer (String peerID)
    {
        newPeer (_peerStatusListeners, peerID);
    }

    public static void newPeer (Collection<PeerStatusListener> listeners, String peerID)
    {
        if (listeners.isEmpty()) {
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.INFO, "DisseminationServiceProxy::newPeer: error:DisseminationServiceProxyListener is null");
            return;
        }

        for (PeerStatusListener listener : listeners) {
            try {
                listener.newPeer(peerID);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    public void deadPeer (String peerID)
    {
        deadPeer (_peerStatusListeners, peerID);
    }

    public static void deadPeer (Collection<PeerStatusListener> listeners, String peerID)
    {
        if (listeners.isEmpty()) {
            System.out.println ("DisseminationServiceProxy::deadPeer: error:DisseminationServiceProxyListener is null");
            return;
        }

        for (PeerStatusListener listener : listeners) {
            try {
                listener.deadPeer (peerID);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    public void searchArrived (String queryId, String groupName, String querier, String queryType,
                                  String queryQualifiers, byte[] query)
    {
        searchArrived (_searchListeners, queryId, groupName, querier, queryType, queryQualifiers, query);
    }

    public static void searchArrived (Collection<SearchListener> listeners, String queryId, String groupName,
                                         String querier, String queryType, String queryQualifiers, byte[] query)
    {
        for (SearchListener listener : listeners) {
            try {
                listener.searchArrived (queryId, groupName, querier, queryType, queryQualifiers, query);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    public void searchReplyArrived (String queryId, Collection<String> mathingMessageIds, String respodnerNodeId)
    {
        searchReplyArrived (_searchListeners, queryId, mathingMessageIds, respodnerNodeId);
    }

    public static void searchReplyArrived (Collection<SearchListener> listeners, String queryId, Collection<String> mathingMessageIds, String respodnerNodeId)
    {
        for (SearchListener listener : listeners) {
            try {
                listener.searchReplyArrived (queryId, mathingMessageIds, respodnerNodeId);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    public void searchReplyArrived(String queryId, byte[] reply, String responderNodeId)
    {
        searchReplyArrived (_searchListeners, queryId, reply, responderNodeId);
    }

    public static void searchReplyArrived (Collection<SearchListener> listeners, String queryId, byte[] reply, String responderNodeId)
    {
        for (SearchListener listener : listeners) {
            try {
                listener.searchReplyArrived (queryId, reply, responderNodeId);
            }
            catch (Exception e) {
                Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            }
        }
    }

    protected CommHelper connectToServer (String host, int port)
        throws Exception
    {
        CommHelper commHelper = new CommHelper();
        commHelper.init (new Socket (host, port));
        return commHelper;
    }

    protected int registerProxy (CommHelper ch, CommHelper chCallback, int desiredApplicationId) throws CommException
    {
        try {
            // First register the proxy using the desired application id
            // The ProxyServer will return the assigned application id
            ch.sendLine ("RegisterProxy " + desiredApplicationId);
            String[]  array = ch.receiveRemainingParsed ("OK");
            int applicationId = Short.parseShort (array[0]);

            // Now register the callback using the assigned application id
            chCallback.sendLine ("RegisterProxyCallback " + applicationId);
            chCallback.receiveMatch ("OK");
            return applicationId;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (DisseminationServiceProxy.class.getName()).log(Level.SEVERE, null, e);
            return -1;
        }
    }

    public void notifyConnectionLoss()
    {
        _isInitialized.set (false);
        for (int i = 0 ; i < _connectionStatusListeners.size() ; i++) {
            _connectionStatusListeners.get(i).connectionLost();
        }
    }

    void setCallbackThreadId (long callbackTheadId)
    {
        _callbackTheadId = callbackTheadId;
    }

    protected void checkConcurrentModification (String exceptionMsg)
    {
        if (Thread.currentThread().getId() == _callbackTheadId) {
            throw new ConcurrentModificationException (exceptionMsg);
        }
    }

    public static final String DEFAULT_HOST  = "localhost";
    public static final int    DIS_SVC_PROXY_SERVER_PORT_NUMBER = 56487;

    protected CommHelper _commHelper;
    protected final AtomicBoolean _isInitialized = new AtomicBoolean (false);
    protected final Map<Short, DisseminationServiceProxyListener> _listeners = new HashMap<Short, DisseminationServiceProxyListener>();
    private final List<PeerStatusListener> _peerStatusListeners = new ArrayList<PeerStatusListener>();
    private final List<SearchListener> _searchListeners = new ArrayList<SearchListener>();
    private final List<ConnectionStatusListener> _connectionStatusListeners = new ArrayList<ConnectionStatusListener>();
    private final CallbackHandlerFactory _handlerFactory;

    private int _port;
    private String _host;
    private String _nodeId;
    private short _applicationId = 0;
    private long _callbackTheadId;
    private long _reinitializationAttemptInterval;
    private DisseminationServiceProxyCallbackHandler _handler;
}
