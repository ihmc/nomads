/*
 * QueuedDisseminationServiceProxy.java
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

import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.concurrent.atomic.AtomicBoolean;

import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class QueuedDisseminationServiceProxy implements DisseminationServiceInterface, DisseminationServiceProxyListener
{
    //--------------------------------------------------------------------------
    // Queued Data Wrappers
    //--------------------------------------------------------------------------

    public class QueuedData
    {
        QueuedData (String msgId, String sender, String groupName, int seqNum, String objectId,
                    String instanceId, String mimeType, byte[] data, short tag,
                    byte priority, String queryId)
        {
            this.msgId = msgId;
            this.sender = sender;
            this.groupName = groupName;
            this.seqNum = seqNum;
            this.objectId = objectId;
            this.instanceId = instanceId;
            this.mimeType = mimeType;
            this.data = data;
            this.tag = tag;
            this.priority = priority;
            this.queryId = queryId;
        }

        public final String msgId;
        public final String sender;
        public final String groupName;
        public final int seqNum;
        public final String objectId;
        public final String instanceId;
        public final String mimeType;
        public final byte[] data;
        public final short tag;
        public final byte priority;
        public final String queryId;
    }

    public class QueuedArrivedData extends QueuedData
    {
        QueuedArrivedData (String msgId, String sender, String groupName, int seqNum, String objectId,
                           String instanceId, String mimeType, byte[] data, int metadataLength,
                           short tag, byte priority, String queryId)
        {
            super (msgId, sender, groupName, seqNum, objectId, instanceId, mimeType, data, tag, priority, queryId);
            this.metadataLength = metadataLength;
        }

        public final int metadataLength;
    }

    public class QueuedArrivedChunk extends QueuedData
    {
        QueuedArrivedChunk (String msgId, String sender, String groupName, int seqNum, String objectId,
                            String instanceId, String mimeType, byte[] data,
                            short nChunks, short totNChunks, String chunhkedMsgId,
                            short tag, byte priority, String queryId)
        {
            super (msgId, sender, groupName, seqNum, objectId, instanceId, mimeType,
                   data, tag, priority, queryId);
            this.nChunks = nChunks;
            this.totNChunks = totNChunks;
            this.chunhkedMsgId = chunhkedMsgId;
        }

        public final short nChunks;
        public final short totNChunks;
        public final String chunhkedMsgId;
    }

    public class QueuedArrivedMetadata extends QueuedData
    {
        QueuedArrivedMetadata (String msgId, String sender, String groupName, int seqNum, String objectId,
                               String instanceId, String mimeType, byte[] data,
                               boolean chunkedData, short tag, byte priority, String queryId)
        {
            super (msgId, sender, groupName, seqNum, objectId, instanceId, mimeType,
                   data, tag, priority, queryId);
            this.chunkedData = chunkedData;
        }

        public final boolean chunkedData;
    }

    public class QueuedAvailableData extends QueuedData
    {
        QueuedAvailableData (String msgId, String sender, String groupName, int seqNum,
                             String objectId, String instanceId, String mimeType, String refObjId,
                             byte[] data, short tag, byte priority, String queryId)
        {
            super (msgId, sender, groupName, seqNum, objectId, instanceId, mimeType,
                   data, tag, priority, queryId);
            this.refObjId = refObjId;
        }

        public final String refObjId;
    }

    public class PeerListUpdate
    {
        public final LinkedList<String> _newPeersList;
        public final LinkedList<String> _deadPeersList;

        PeerListUpdate (Queue newPeers, Queue deadPeers)
        {
            _newPeersList = new LinkedList<String> (newPeers);
            _deadPeersList = new LinkedList<String> (deadPeers);
        }
    }

    public class ArrivedSearch
    {
        public final String _queryId;
        public final String _groupName;
        public final String _querier;
        public final String _queryType;
        public final String _queryQualifiers;
        public final byte[] _query;

        ArrivedSearch (String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query)
        {
            _queryId = queryId;
            _groupName = groupName;
            _querier = querier;
            _queryType = queryType;
            _queryQualifiers = queryQualifiers;
            _query = query;
        }
    }

    public class ArrivedSearchReply
    {
        public final String _queryId;
        public final Collection<String> _matchingIds;
        public final String _responderNodeId;

        ArrivedSearchReply (String queryId, Collection<String> matchingIds, String responderNodeId)
        {
            _queryId = queryId;
            _matchingIds = matchingIds;
            _responderNodeId = responderNodeId;
        }
    }

    public class ArrivedVolatileSearchReply
    {
        public final String _queryId;
        public final byte[] _reply;
        public final String _responderNodeId;

        ArrivedVolatileSearchReply (String queryId, byte[] reply, String responderNodeId)
        {
            _queryId = queryId;
            _reply = reply;
            _responderNodeId = responderNodeId;
        }
    }

    //--------------------------------------------------------------------------
    // QueuedDisseminationServiceProxy
    //--------------------------------------------------------------------------

    public QueuedDisseminationServiceProxy()
    {
        this (new DisseminationServiceProxy());
    }

    public QueuedDisseminationServiceProxy (short applicationId)
    {
        this (new DisseminationServiceProxy (applicationId));
    }

    public QueuedDisseminationServiceProxy (short applicationId, long reinitializationAttemptInterval)
    {
        this (new DisseminationServiceProxy (applicationId, reinitializationAttemptInterval));
    }

    public QueuedDisseminationServiceProxy (short applicationId, String host, int iPort)
    {
    	this (new DisseminationServiceProxy (applicationId, host, iPort));
    }

    public QueuedDisseminationServiceProxy (short applicationId, String host, int port,
                                            long reinitializationAttemptInterval)
    {
        this (new DisseminationServiceProxy (applicationId, host, port, reinitializationAttemptInterval));
    }

    protected QueuedDisseminationServiceProxy (DisseminationServiceProxy proxy)
    {
        _proxy = proxy;
    }

    public int init() throws Exception
    {
        return _proxy.init();
    }

    public void configure (boolean bQueueArrivedData, boolean bQueueArrivedChunks,
                           boolean bQueueArrivedMetadata, boolean bQueueAvailableData)
    {
        _queueArrivedData.set (bQueueArrivedData);
        _queueArrivedChunks.set (bQueueArrivedChunks);
        _queueArrivedMetadata.set (bQueueArrivedMetadata);
        _queueAvailableData.set (bQueueAvailableData);
    }

    @Override
    public synchronized void dataArrived (String msgId, String sender, String groupName, int seqNum, String objectId,
                                          String instanceId, String mimeType, byte[] data,
                                          int metadataLength, short tag, byte priority,
                                          String queryId)
    {
        if (_queueArrivedData.get()) {
            _arrivedData.add (new QueuedArrivedData (msgId, sender, groupName, seqNum, objectId,
                                                     instanceId, mimeType, data, metadataLength,
                                                     tag, priority, queryId));
        }
        notifyAll();
    }

    @Override
    public synchronized void chunkArrived (String msgId, String sender, String groupName, int seqNum, String objectId,
                                           String instanceId, String mimeType, byte[] data,
                                           short nChunks, short totNChunks, String chunhkedMsgId,
                                           short tag, byte priority, String queryId)
    {
        if (_queueArrivedChunks.get()) {
            _arrivedChunk.add (new QueuedArrivedChunk (msgId, sender, groupName, seqNum, objectId,
                                                       instanceId, mimeType, data, nChunks, totNChunks,
                                                       chunhkedMsgId, tag, priority, queryId));
        }
        notifyAll();
    }

    @Override
    public synchronized void metadataArrived (String msgId, String sender, String groupName, int seqNum,
                                              String objectId, String instanceId, String mimeType,
                                              byte[] data, boolean dataChunked, short tag,
                                              byte priority, String queryId)
    {
        if (_queueArrivedMetadata.get()) {
            _arrivedMetadata.add (new QueuedArrivedMetadata (msgId, sender, groupName, seqNum, objectId,
                                                             instanceId, mimeType, data, dataChunked, tag,
                                                             priority, queryId));
        }
        notifyAll();
    }

    @Override
    public synchronized void dataAvailable (String msgId, String sender, String groupName, int seqNum, String objectId,
                                  String instanceId, String mimeType, String refObjId,
                                  byte[] data, short tag, byte priority, String queryId)
    {
        if (_queueAvailableData.get()) {
            _availableData.add (new QueuedAvailableData (msgId, sender, groupName, seqNum, objectId,
                                                         instanceId, mimeType, refObjId, data, tag, priority,
                                                         queryId));
        }
        notifyAll();
    }

    public synchronized void searchArrived (String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query)
    {
        if (_queueArrivedSearch.get()) {
            _arrivedSearches.add (new ArrivedSearch (queryId, groupName, querier, queryType, queryQualifiers, query));
        }
        notifyAll();
    }

    public synchronized void searchReplyArrived (String queryId, Collection<String> matchingNodeIds, String responderNodeId)
    {
        if (_queueArrivedSearchReply.get()) {
            _arrivedSearchReply.add (new ArrivedSearchReply (queryId, matchingNodeIds, responderNodeId));
        }
        notifyAll();
    }

    public synchronized void searchReplyArrived (String queryId, byte[] reply, String responderNodeId)
    {
        if (_queueArrivedVolatileSearchReply.get()) {
            _arrivedVolatileSearchReply.add (new ArrivedVolatileSearchReply (queryId, reply, responderNodeId));
        }
        notifyAll();
    }

    public synchronized void waitIfEmpty()
    {
        if (_queueArrivedData.get() && !_arrivedData.isEmpty()) {
            return;
        }
        if (_queueArrivedChunks.get() && !_arrivedChunk.isEmpty()) {
            return;
        }
        if (_queueArrivedMetadata.get() && !_arrivedMetadata.isEmpty()) {
            return;
        }
        if (_queueAvailableData.get() && !_availableData.isEmpty()) {
            return;
        }
        if (_queueArrivedSearch.get() && !_arrivedSearches.isEmpty()) {
            return;
        }
        if (_queueArrivedSearchReply.get() && !_arrivedSearchReply.isEmpty()) {
            return;
        }
        if (_queueArrivedVolatileSearchReply.get() && !_arrivedVolatileSearchReply.isEmpty()) {
            return;
        }
        if (!_newPeers.isEmpty() || !_deadPeers.isEmpty()) {
            return;
        }
        try {
            wait();
        }
        catch (InterruptedException ex) {}
    }

    /**
     * Retrieves and removes the head of the arrived data queue, or returns null
     * if this queue is empty.
     * @return 
     */
    public synchronized QueuedArrivedData getArrivedData()
    {
        return _arrivedData.poll();
    }

    /**
     * Retrieves and removes the head of the arrived chunk queue, or returns
     * null if this queue is empty.
     * @return 
     */
    public synchronized QueuedArrivedChunk getArrivedChunk()
    {
        return _arrivedChunk.poll();
    }

    /**
     * Retrieves and removes the head of the arrived metadata queue, or returns
     * null if this queue is empty.
     * @return 
     */
    public synchronized QueuedArrivedMetadata getArrivedMetadata()
    {
        return _arrivedMetadata.poll();
    }

    /**
     * Retrieves and removes the head of the available data queue, or returns
     * null if this queue is empty.
     * @return 
     */
    public synchronized QueuedAvailableData getAvailableData()
    {
        return _availableData.poll();
    }

    public synchronized PeerListUpdate getPeerListUpdate()
    {
        synchronized (_newPeers) {
            if (_newPeers.isEmpty() && _deadPeers.isEmpty()) {
                return null;
            }
            PeerListUpdate lu = new PeerListUpdate (_newPeers, _deadPeers);
            _newPeers.clear();
            _deadPeers.clear();
            return lu;
        }
    }

    /**
     * Retrieves and removes the head of the arrived search queue, or returns
     * null if this queue is empty.
     * @return 
     */
    public synchronized ArrivedSearch getArrivedSearch()
    {
        return _arrivedSearches.poll();
    }

    public synchronized ArrivedSearchReply getArrivedSearchReply()
    {
        return _arrivedSearchReply.poll();
    }

    public synchronized ArrivedVolatileSearchReply getArrivedVolatileSearchReply()
    {
        return _arrivedVolatileSearchReply.poll();
    }

    public void reinitialize()
    {
        _proxy.reinitialize();
    }

    public boolean isInitialized()
    {
        return _proxy.isInitialized();
    }

    public String getNodeId() throws CommException
    {
        return _proxy.getNodeId();
    }

    public List<String> getPeerList()
    {
        return _proxy.getPeerList();
    }

    public String store (String groupName, String objectId, String instanceId,
                         String mimeType, byte[] metaData, byte[] data, long expiration,
                         short historyWindow, short tag, byte priority) throws CommException
    {
        return _proxy.store (groupName, objectId, instanceId, mimeType, metaData, data,
                             expiration, historyWindow, tag, priority);
    }

    public void push (String msgId) throws CommException
    {
        _proxy.push (msgId);
    }

    public String push (String groupName, String objectId, String instanceId, String mimeType,
                        byte[] metaData, byte[] data, long expiration, short historyWindow,
                        short tag, byte priority)
        throws CommException
    {
        return _proxy.push (groupName, objectId, instanceId, mimeType, metaData, data,
                            expiration, historyWindow, tag, priority);
    }

    public String makeAvailable (String groupName, String objectId, String instanceId, byte[] metadata,
                                 byte[] data, String dataMimeType, long expiration, short historyWindow,
                                 short tag, byte priority)
        throws CommException
    {
        return _proxy.makeAvailable (groupName, objectId, instanceId, metadata, data, dataMimeType,
                                     expiration, historyWindow, tag, priority);
    }

    public void cancel(String id)
        throws CommException, ProtocolException
    {
        _proxy.cancel(id);
    }

    public void cancel(short tag)
        throws CommException, ProtocolException
    {
        _proxy.cancel(tag);
    }

    public boolean addFilter(String groupName, short tag)
        throws CommException
    {
        return _proxy.addFilter(groupName, tag);
    }

    public boolean removeFilter(String groupName, short tag)
        throws CommException
    {
        return _proxy.removeFilter(groupName, tag);
    }

    public boolean requestMoreChunks(String groupName, String senderNodeId, int seqId)
        throws CommException
    {
        return _proxy.requestMoreChunks(groupName, senderNodeId, seqId);
    }

    public boolean requestMoreChunks(String messageId)
        throws CommException
    {
        return _proxy.requestMoreChunks(messageId);
    }

    public byte[] retrieve(String id, int timeout)
            throws CommException
    {
        return _proxy.retrieve(id, timeout);
    }

    public int retrieve(String id, String filePath)
    {
        return _proxy.retrieve(id, filePath);
    }

    public boolean request(String groupName, short tag, short historyLength, long timeout)
        throws CommException
    {
        return _proxy.request(groupName, tag, historyLength, timeout);
    }

    public String search(String groupName, String queryType, String queryQualifiers, byte[] query)
        throws CommException
    {
        return _proxy.search (groupName, queryType, queryQualifiers, query);
    }

    public void replyToSearch(String queryId, Collection<String> disServiceMsgIds)
        throws CommException
    {
        _proxy.replyToSearch (queryId, disServiceMsgIds);
    }

    public boolean subscribe(String groupName, byte priority, boolean groupReliable, boolean msgReliable, boolean sequenced)
        throws CommException
    {
        return _proxy.subscribe(groupName, priority, groupReliable, msgReliable, sequenced);
    }

    public boolean subscribe(String groupName, short tag, byte priority, boolean groupReliable, boolean msgReliable, boolean sequenced)
        throws CommException
    {
        return _proxy.subscribe(groupName, tag, priority, groupReliable, msgReliable, sequenced);
    }

    public boolean subscribe(String groupName, byte predicateType, String predicate, byte priority, boolean groupReliable, boolean msgReliable, boolean sequenced)
        throws CommException
    {
        return _proxy.subscribe(groupName, predicateType, predicate, priority, groupReliable, msgReliable, sequenced);
    }

    public boolean unsubscribe(String groupName)
        throws CommException
    {
        return _proxy.unsubscribe(groupName);
    }

    public boolean unsubscribe(String groupName, short tag)
        throws CommException
    {
        return _proxy.unsubscribe (groupName, tag);
    }

    public void registerDisseminationServiceProxyListener(DisseminationServiceProxyListener listener)
        throws CommException
    {
        _proxy.registerDisseminationServiceProxyListener (listener);
    }

    public void registerDisseminationServiceProxyListener (short clientId, DisseminationServiceProxyListener listener)
        throws ListenerAlreadyRegisteredException
    {
        _proxy.registerDisseminationServiceProxyListener (clientId, listener);
    }

    public void registerPeerStatusListener(PeerStatusListener listener)
    {
        _proxy.registerPeerStatusListener (listener);
    }

    public void registerConnectionStatusListener(ConnectionStatusListener listener)
    {
        _proxy.registerConnectionStatusListener (listener);
    }

    public void registerSearchListener(SearchListener listener)
    {
        _proxy.registerSearchListener (listener);
    }

    public boolean resetTransmissionHistory()
        throws CommException
    {
        return _proxy.resetTransmissionHistory();
    }

    public synchronized void newPeer(String peerID)
    {
        synchronized (_newPeers) {
            _deadPeers.remove (peerID);
            _newPeers.add (peerID);
        }
        notifyAll();
    }

    public synchronized void deadPeer(String peerID)
    {
        synchronized (_newPeers) {
            _newPeers.remove (peerID);
            _deadPeers.add (peerID);
        }
        notifyAll();
    }

    private final AtomicBoolean _queueArrivedData = new AtomicBoolean (true);
    private final AtomicBoolean _queueArrivedChunks = new AtomicBoolean (true);
    private final AtomicBoolean _queueArrivedMetadata = new AtomicBoolean (true);
    private final AtomicBoolean _queueAvailableData = new AtomicBoolean (true);
    private final AtomicBoolean  _queueArrivedSearch = new AtomicBoolean (true);
    private final AtomicBoolean  _queueArrivedSearchReply = new AtomicBoolean (true);
    private final AtomicBoolean  _queueArrivedVolatileSearchReply = new AtomicBoolean (true);

    private final Queue<QueuedArrivedData> _arrivedData = new LinkedList<QueuedArrivedData>();
    private final Queue<QueuedAvailableData> _availableData = new LinkedList<QueuedAvailableData>();
    private final Queue<QueuedArrivedChunk> _arrivedChunk = new LinkedList<QueuedArrivedChunk>();
    private final Queue<QueuedArrivedMetadata> _arrivedMetadata = new LinkedList<QueuedArrivedMetadata>();
    private final Queue<ArrivedSearch> _arrivedSearches = new LinkedList<ArrivedSearch>();
    private final Queue<ArrivedSearchReply> _arrivedSearchReply = new LinkedList<ArrivedSearchReply>();
    private final Queue<ArrivedVolatileSearchReply> _arrivedVolatileSearchReply = new LinkedList<ArrivedVolatileSearchReply>();

    private final Queue<String> _newPeers = new LinkedList<String>();
    private final Queue<String> _deadPeers = new LinkedList<String>();

    protected final DisseminationServiceProxy _proxy;
}
