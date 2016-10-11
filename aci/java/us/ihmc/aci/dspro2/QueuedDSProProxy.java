/*
 * QueuedDSProProxy.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import java.util.Collection;
import java.util.Iterator;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import us.ihmc.aci.dspro2.util.LoggerInterface;
import us.ihmc.aci.dspro2.util.LoggerWrapper;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class QueuedDSProProxy extends DSProProxy
{
    public interface Callback
    {        
    }

    //--------------------------------------------------------------------------
    // Queued Data Wrappers
    //--------------------------------------------------------------------------

    public class QueuedData implements Callback
    {
        QueuedData (String id, String groupName, String objectId,
                    String instanceId, String queryId)
        {
            this.id = id;
            this.groupName = groupName;
            this.objectId = objectId;
            this.instanceId = instanceId;
            this.queryId = queryId;
        }

        public final String id;
        public final String groupName;
        public final String objectId;
        public final String instanceId;
        public final String queryId;
    }

    public class QueuedArrivedData extends QueuedData
    {
        QueuedArrivedData (String id, String groupName, String objectId,
                           String instanceId, String annotatedObjMsgId,
                           String mimeType, byte[] data, short chunkNumber,
                           short totChunksNumber, String queryId)
        {
            super (id, groupName, objectId, instanceId, queryId);
            this.annotatedObjMsgId = annotatedObjMsgId;
            this.mimeType = mimeType;
            this.data = data;
            this.chunkNumber = chunkNumber;
            this.totChunksNumber = totChunksNumber;
        }

        public final String annotatedObjMsgId;
        public final String mimeType;
        public final byte[] data;
        public final short chunkNumber;
        public final short totChunksNumber;
    }

    public class QueuedArrivedMetadata extends QueuedData
    {
        QueuedArrivedMetadata (String id, String groupName, String objectId,
                               String instanceId, String XMLMetadata,
                               String referredDataId, String queryId)
        {
            super (id, groupName, objectId, instanceId, queryId);
            this.XMLMetadata = XMLMetadata;
            this.referredDataId = referredDataId;
        }

        public final String XMLMetadata;
        public final String referredDataId;
    }

    public class QueuedArrivedPeer implements Callback
    {
        public final String _nodeId;

        QueuedArrivedPeer (String nodeId)
        {
            this._nodeId = nodeId;
        }
    }

    public class QueuedDeadPeer implements Callback
    {
        public final String _nodeId;

        QueuedDeadPeer (String nodeId)
        {
            this._nodeId = nodeId;
        }
    }

    public class QueuedRegisteredPath implements Callback
    {
        public final NodePath _path;
        public final String _nodeId;
        public final String _teamId;
        public final String _mission;

        QueuedRegisteredPath (NodePath path, String nodeId, String teamId, String mission)
        {
            _path = path;
            this._nodeId = nodeId;
            _teamId = teamId;
            _mission = mission;
        }
    }

    public class QueuedUpdatedPosition implements Callback
    {
        public final float _latitude;
        public final float _longitude;
        public final float _altitude;
        public final String _nodeId;

        QueuedUpdatedPosition (float latitude, float longitude, float altitude, String nodeId)
        {
            _latitude = latitude;
            _longitude = longitude;
            _altitude = altitude;
            this._nodeId = nodeId;
        }
    }

    public class QueuedArrivedSearch implements Callback
    {
        public final String _queryId;
        public final String _groupName;
        public final String _querier;
        public final String _queryType;
        public final String _queryQualifiers;
        public final byte[] _query;

        public QueuedArrivedSearch (String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query)
        {
            _queryId = queryId;
            _groupName = groupName;
            _querier = querier;
            _queryType = queryType;
            _queryQualifiers = queryQualifiers;
            _query = query;
        }
    }

    public class QueuedArrivedSearchReply implements Callback
    {
        public final String _queryId;
        public final Collection<String> _matchingIds;
        public final String _responderNodeId;

        QueuedArrivedSearchReply (String queryId, Collection<String> matchingIds, String responderNodeId)
        {
            _queryId = queryId;
            _matchingIds = matchingIds;
            _responderNodeId = responderNodeId;
        }
    }

    public class QueuedArrivedVolatileSearchReply implements Callback
    {
        public final String _queryId;
        public final byte[] _reply;
        public final String _responderNodeId;

        QueuedArrivedVolatileSearchReply (String queryId, byte[] reply, String responderNodeId)
        {
            _queryId = queryId;
            _reply = reply;
            _responderNodeId = responderNodeId;
        }
    }

    //--------------------------------------------------------------------------
    // Class implementation
    //--------------------------------------------------------------------------

    public QueuedDSProProxy ()
    {
        super();
    }

    public QueuedDSProProxy (short applicationId)
    {
        super (applicationId);
    }

    public QueuedDSProProxy (short applicationId, String host, int iPort)
    {
    	super (applicationId, host, iPort);
    }

    @Override
    public boolean dataArrived (String id, String groupName, String objectId,
                                String instanceId, String annotatedObjMsgId,
                                String mimeType, byte[] data, short chunkNumber,
                                short totChunksNumber, String queryId)
    {
        if (_nDSProListeners.get() <= 0) {
            return true;
        }
        _arrivedDSProListenerCallback.add (new QueuedArrivedData (id, groupName, objectId,
                                           instanceId, annotatedObjMsgId, mimeType, data,
                                           chunkNumber, totChunksNumber, queryId));
        return true;
    }

    @Override
    public boolean metadataArrived (String id, String groupName, String objectId, String instanceId,
                                    String XMLMetadata, String referredDataId, String queryId)
    {
        if (_nDSProListeners.get() <= 0) {
            return true;
        }
        
        System.out.println ("NOMADS:QueuedDSProProxy: metadataArrived: " + id + " about to adding it to the queue");
        
        _arrivedDSProListenerCallback.add (new QueuedArrivedMetadata (id, groupName, objectId, instanceId,
                                                                      XMLMetadata, referredDataId, queryId));

        System.out.println ("NOMADS:QueuedDSProProxy: metadataArrived: " + id
                + " added it to the queue (queue size: " + _arrivedDSProListenerCallback.size() + ")");

        return true;
    }

    @Override
    public void searchArrived (String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query)
    {
        _arrivedDSProListenerCallback.add (new QueuedArrivedSearch (queryId, groupName, querier, queryType, queryQualifiers, query));
    }

    @Override
    public void searchReplyArrived (String queryId, Collection<String> matchingIds, String responderNodeId)
    {
        _arrivedDSProListenerCallback.add (new QueuedArrivedSearchReply (queryId, matchingIds, responderNodeId));
    }

    @Override
    public void searchReplyArrived (String queryId, byte[] reply, String responderNodeId)
    {
        _arrivedDSProListenerCallback.add (new QueuedArrivedVolatileSearchReply (queryId, reply, responderNodeId));
    }

    @Override
    public void newNeighbor (String peerID)
    {
        if (_nDSProListeners.get() <= 0) {
            return;
        }

        synchronized (_arrivedDSProListenerCallback) {
            Iterator<Callback> iter = _arrivedDSProListenerCallback.iterator();
            while (iter.hasNext()) {
                Callback cback = iter.next();
                if ((cback instanceof QueuedDeadPeer) &&
                        (((QueuedDeadPeer) cback)._nodeId.compareTo (peerID) == 0)) {
                    iter.remove();
                }
                else if ((cback instanceof QueuedArrivedPeer) &&
                        (((QueuedArrivedPeer) cback)._nodeId.compareTo (peerID) == 0)) {
                    iter.remove();
                }
            }
            _arrivedDSProListenerCallback.add (new QueuedArrivedPeer (peerID));
        }
    }

    @Override
    public void deadNeighbor (String peerID)
    {
        if (_nDSProListeners.get() <= 0) {
            return;
        }

        synchronized (_arrivedDSProListenerCallback) {
            Iterator<Callback> iter = _arrivedDSProListenerCallback.iterator();
            while (iter.hasNext()) {
                Callback cback = iter.next();
                if ((cback instanceof QueuedArrivedPeer) &&
                        (((QueuedArrivedPeer) cback)._nodeId.compareTo (peerID) == 0)) {
                    iter.remove();
                }
                else if ((cback instanceof QueuedDeadPeer) &&
                        (((QueuedDeadPeer) cback)._nodeId.compareTo (peerID) == 0)) {
                    iter.remove();
                }
            }
            _arrivedDSProListenerCallback.add (new QueuedDeadPeer (peerID));
        }
    }

    @Override
    public boolean pathRegistered (NodePath path, String nodeId, String teamId, String mission)
    {
        if (_nDSProListeners.get() <= 0) {
            return true;
        }

        _arrivedDSProListenerCallback.add (new QueuedRegisteredPath (path, nodeId, teamId, mission));
        return true;
    }

    @Override
    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId)
    {
        if (_nDSProListeners.get() <= 0) {
            return true;
        }

        _arrivedDSProListenerCallback.add (new QueuedUpdatedPosition (latitude, longitude, altitude, nodeId));
        return true;
    }

    /**
     * Retrieves and removes the head of the arrived data queue, or returns null
     * if this queue is empty.
     */
    public Callback getDSProListenerCallback()
    {
        return _arrivedDSProListenerCallback.poll();
    }

    /**
     * Retrieves and removes the head of this queue, waiting up to the specified
     * wait time if necessary for an element to become available.
     *
     * @param timeout how long to wait before giving up, in units of unit
     * @param unit  a TimeUnit determining how to interpret the timeout parameter
     * @return the head of this queue, or null if the specified waiting time elapses
     * before an element is available
     * @throws InterruptedException 
     */
    public Callback getDSProListenerCallback (long timeout, TimeUnit unit)
        throws InterruptedException
    {
        return _arrivedDSProListenerCallback.poll (timeout, unit);
    }

    @Override
    public int registerDSProProxyListener (DSProProxyListener listener)
        throws CommException
    {
        LOG.info ("RegisterDSProProxyListener -> number of registered listeners: " + _nDSProListeners.get());
        int rc = 0;
        if (_nDSProListeners.get() == 0) {
            rc = super.registerDSProProxyListener (this);
        }
        _nDSProListeners.incrementAndGet();
        return rc;
    }

    @Override
    public void deregisterDSProProxyListener (DSProProxyListener listener)
        throws CommException
    {
        LOG.info ("DeregisterDSProProxyListener -> number of registered listeners: " + _nDSProListeners.get());
        _nDSProListeners.decrementAndGet();
        LOG.info ("DeregisterDSProProxyListener -> number of registered listeners after remove: " + _nDSProListeners.get());
        if (_nDSProListeners.get() == 0) {
            super.deregisterDSProProxyListener(this);
        }
    }

    @Override
    public int registerSearchListener (SearchListener listener)
        throws CommException
    {
        int rc = 0;
        if (_nSearchListeners.get() == 0) {
            rc = super.registerSearchListener (this);
        }
        _nSearchListeners.incrementAndGet();
        return rc;
    }

    @Override
    public void deregisterSearchListener (SearchListener listener)
        throws CommException
    {
        _nSearchListeners.decrementAndGet();
        if (_nSearchListeners.get() == 0) {
            super.deregisterSearchListener(this);
        }
    }

    private final AtomicInteger _nDSProListeners = new AtomicInteger (0);
    private final AtomicInteger _nSearchListeners = new AtomicInteger (0);
    private final BlockingQueue<Callback> _arrivedDSProListenerCallback = new LinkedBlockingQueue<Callback>();

    private final static LoggerInterface LOG = LoggerWrapper.getLogger (QueuedDSProProxy.class);
}
