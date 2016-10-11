/*
 * AsyncDSProProxy.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import us.ihmc.aci.dspro2.util.LoggerInterface;
import us.ihmc.aci.dspro2.util.LoggerWrapper;
import us.ihmc.comm.CommException;

import us.ihmc.util.StringUtil;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class AsyncDSProProxy extends QueuedDSProProxy implements Runnable
{
    public static final long DEFAULT_POLLING_TIME = 5000;

    public AsyncDSProProxy()
    {
        this (DEFAULT_POLLING_TIME);
    }

    public AsyncDSProProxy (long pollingTime)
    {
        super();
    }

    public AsyncDSProProxy (short applicationId)
    {
        this (applicationId, DEFAULT_POLLING_TIME);
    }

    public AsyncDSProProxy (short applicationId, long pollingTime)
    {
        super (applicationId);
    }

    public AsyncDSProProxy (short applicationId, String host, int iPort)
    {
    	this (applicationId, host, iPort, DEFAULT_POLLING_TIME);
    }

    public AsyncDSProProxy (short applicationId, String host, int iPort, long pollingTime)
    {
    	super (applicationId, host, iPort);
    }

    public void run()
    {
        Thread.currentThread().setName (AsyncDSProProxy.class.getSimpleName());

        while (true) {
            try {
                // Using TimeUnit.SECONDS instead of TimeUnit.DAYS because some older versions
                // of dalvikvm (android) do not seeem to support it! 
                Callback cback = getDSProListenerCallback (Long.MAX_VALUE, TimeUnit.SECONDS);
                if (cback != null) {
                    if (cback instanceof QueuedArrivedData) {
                        QueuedArrivedData ad = (QueuedArrivedData) cback;
                        DSProProxy.dataArrived (_dsProListeners, ad.id, ad.groupName, ad.objectId,
                                                ad.instanceId, ad.annotatedObjMsgId, ad.mimeType,
                                                ad.data, ad.chunkNumber, ad.totChunksNumber, ad.queryId);
                    }
                    else if (cback instanceof QueuedArrivedMetadata) {
                        QueuedArrivedMetadata am = (QueuedArrivedMetadata) cback;
                        System.out.println ("NOMADS:AsynchDSProProxy: metadataArrived: " + am.id + " about to notify clients");
                        LOG.info ("NOMADS:AsynchDSProProxy: metadataArrived: " + am.id + " about to notify clients");
                        DSProProxy.metadataArrived (_dsProListeners, am.id, am.groupName,
                                                  am.objectId, am.instanceId,
                                                  am.XMLMetadata, am.referredDataId,
                                                  am.queryId);
                        System.out.println ("NOMADS:AsynchDSProProxy: metadataArrived: " + am.id + "notified clients");
                        LOG.info ("NOMADS:AsynchDSProProxy: metadataArrived: " + am.id + "notified clients");
                    }
                    else if (cback instanceof QueuedRegisteredPath) {
                        QueuedRegisteredPath ap = (QueuedRegisteredPath) cback;
                        DSProProxy.pathRegistered (_dsProListeners, ap._path, ap._nodeId,
                                                 ap._teamId, ap._mission);
                    }
                    else if (cback instanceof QueuedUpdatedPosition) {
                        QueuedUpdatedPosition rp = (QueuedUpdatedPosition) cback;
                        DSProProxy.positionUpdated (_dsProListeners, rp._latitude, rp._longitude,
                                                  rp._altitude, rp._nodeId);
                    }
                    else if (cback instanceof QueuedArrivedPeer) {
                        QueuedArrivedPeer ap = (QueuedArrivedPeer) cback;
                        DSProProxy.newNeighbor (_dsProListeners, ap._nodeId);
                    }
                    else if (cback instanceof QueuedDeadPeer) {
                        QueuedDeadPeer dp = (QueuedDeadPeer) cback;
                        DSProProxy.deadNeighbor (_dsProListeners, dp._nodeId);
                    }
                    else if (cback instanceof QueuedArrivedSearch) {
                        QueuedArrivedSearch as = (QueuedArrivedSearch) cback;
                        DSProProxy.searchArrived (_searchListeners, as._queryId,
                                                as._groupName, as._querier,
                                                as._queryType, as._queryQualifiers,
                                                as._query);
                    }
                    else if (cback instanceof QueuedArrivedSearchReply) {
                        QueuedArrivedSearchReply asr = (QueuedArrivedSearchReply) cback;
                        DSProProxy.searchReplyArrived (_searchListeners, asr._queryId,
                                                asr._matchingIds, asr._responderNodeId);
                    }
                    else if (cback instanceof QueuedArrivedVolatileSearchReply) {
                        QueuedArrivedVolatileSearchReply avsr = (QueuedArrivedVolatileSearchReply) cback;
                        DSProProxy.searchReplyArrived (_searchListeners, avsr._queryId,
                                                       avsr._reply, avsr._responderNodeId);
                    }
                }
            }
            catch (InterruptedException ex) {}
            catch (Exception ex) {
                // It makes sure that the thread keeps working even if the
                // listener apps do not properly handle exceptions
                LOG.error (StringUtil.getStackTraceAsString (ex));
            }
        }
    }

    @Override
    public synchronized int registerDSProProxyListener (DSProProxyListener listener)
        throws CommException
    {
        LOG.info ("Register method called for DSProProxy in " + AsyncDSProProxy.class.getSimpleName());
        if (listener == null) {
            LOG.warn ("Error:DisseminationServiceProxyListener is null");
        }
        LOG.info (AsyncDSProProxy.class.getSimpleName() + " _dsProListeners size: " + _dsProListeners.size());
        if (_dsProListeners.isEmpty()) {
            super.registerDSProProxyListener(this);
        }
        _dsProListeners.add (listener);
        return _dsProListeners.size()-1;
    }

    @Override
    public synchronized void deregisterDSProProxyListener (DSProProxyListener listener)
        throws CommException
    {
        LOG.info ("Deregister method called for DSProProxy in " + AsyncDSProProxy.class.getSimpleName());
        if (listener == null) {
            LOG.warn ("Error:DisseminationServiceProxyListener is null");
            return;
        }
        _dsProListeners.remove (listener);
        if (_dsProListeners.isEmpty()) {
            super.deregisterDSProProxyListener(this);
        }
    }

    @Override
    public synchronized int registerSearchListener (SearchListener listener)
        throws CommException
    {
        if (listener == null) {
            LOG.warn ("Error:DisseminationServiceProxyListener is null");
            return -1;
        }
        if (_searchListeners.isEmpty()) {
            super.registerSearchListener (this);
        }
        _searchListeners.add (listener);
        return _searchListeners.size()-1;
    }

    @Override
    public synchronized void deregisterSearchListener (SearchListener listener)
        throws CommException
    {
        if (listener == null) {
            LOG.warn ("Error:DisseminationServiceProxyListener is null");
            return;
        }
        _searchListeners.remove (listener);
        if (_searchListeners.isEmpty()) {
            super.deregisterSearchListener (this);
        }
    }

    private final List<DSProProxyListener> _dsProListeners = new ArrayList<DSProProxyListener>();
    private final List<SearchListener> _searchListeners = new ArrayList<SearchListener>();

    protected final static LoggerInterface LOG = LoggerWrapper.getLogger (AsyncDSProProxy.class);
}
