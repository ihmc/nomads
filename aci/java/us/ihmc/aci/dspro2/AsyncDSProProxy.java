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
import java.util.concurrent.atomic.AtomicBoolean;

import org.slf4j.Logger;
import us.ihmc.aci.util.dspro.LogUtils;
import us.ihmc.comm.CommException;

import us.ihmc.util.StringUtil;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class AsyncDSProProxy extends AbstractDSProProxy implements Runnable
{
    private static AtomicBoolean _terminate = new AtomicBoolean(false);
    public static final long DEFAULT_POLLING_TIME = 5000;

    public AsyncDSProProxy (QueuedDSProProxy proxy)
    {
        this(proxy, DEFAULT_POLLING_TIME);
    }
    public AsyncDSProProxy (QueuedDSProProxy proxy, long pollingTime)
    {
        super(proxy);
    }

    @Override
    public void run()
    {
        Thread.currentThread().setName (AsyncDSProProxy.class.getSimpleName());

        while (!_terminate.get()) {
            try {
                // Using TimeUnit.SECONDS instead of TimeUnit.DAYS because some older versions
                // of dalvikvm (android) do not seeem to support it!
                QueuedDSProProxy.Callback cback = ((QueuedDSProProxy) proxy).getDSProListenerCallback (Long.MAX_VALUE, TimeUnit.SECONDS);
                if (cback != null) {
                    if (cback instanceof QueuedDSProProxy.QueuedArrivedData) {
                        QueuedDSProProxy.QueuedArrivedData ad = (QueuedDSProProxy.QueuedArrivedData) cback;
                        DSProProxy.dataArrived (_dsProListeners, ad.id, ad.groupName, ad.objectId,
                                                ad.instanceId, ad.annotatedObjMsgId, ad.mimeType,
                                                ad.data, ad.chunkNumber, ad.totChunksNumber, ad.queryId);
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedArrivedMetadata) {
                        QueuedDSProProxy.QueuedArrivedMetadata am = (QueuedDSProProxy.QueuedArrivedMetadata) cback;
                        LOG.info ("NOMADS:AsynchDSProProxy: metadataArrived: " + am.id + " about to notify clients");
                        DSProProxy.metadataArrived (_dsProListeners, am.id, am.groupName,
                                                  am.objectId, am.instanceId,
                                                  am.jsonMetadata, am.referredDataId,
                                                  am.queryId);
                        LOG.info ("NOMADS:AsynchDSProProxy: metadataArrived: " + am.id + "notified clients");
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedAvailableData) {
                        QueuedDSProProxy.QueuedAvailableData ad = (QueuedDSProProxy.QueuedAvailableData) cback;
                        LOG.trace ("NOMADS:AsynchDSProProxy: dataAvailable: " + ad.id + " about to notify clients");
                        DSProProxy.dataAvailable (_dsProListeners, ad.id, ad.groupName, ad.objectId, ad.instanceId,
                                                  ad.referredDataId, ad.mimeType, ad.metadata, ad.queryId);
                        LOG.trace ("NOMADS:AsynchDSProProxy: dataAvailable: " + ad.id + "notified clients");
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedRegisteredPath) {
                        QueuedDSProProxy.QueuedRegisteredPath ap = (QueuedDSProProxy.QueuedRegisteredPath) cback;
                        DSProProxy.pathRegistered (_dsProListeners, ap._path, ap._nodeId,
                                                 ap._teamId, ap._mission);
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedUpdatedPosition) {
                        QueuedDSProProxy.QueuedUpdatedPosition rp = (QueuedDSProProxy.QueuedUpdatedPosition) cback;
                        DSProProxy.positionUpdated (_dsProListeners, rp._latitude, rp._longitude,
                                                  rp._altitude, rp._nodeId);
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedArrivedPeer) {
                        QueuedDSProProxy.QueuedArrivedPeer ap = (QueuedDSProProxy.QueuedArrivedPeer) cback;
                        DSProProxy.newNeighbor (_dsProListeners, ap._nodeId);
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedDeadPeer) {
                        QueuedDSProProxy.QueuedDeadPeer dp = (QueuedDSProProxy.QueuedDeadPeer) cback;
                        DSProProxy.deadNeighbor (_dsProListeners, dp._nodeId);
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedArrivedSearch) {
                        QueuedDSProProxy.QueuedArrivedSearch as = (QueuedDSProProxy.QueuedArrivedSearch) cback;
                        DSProProxy.searchArrived (_searchListeners, as._queryId,
                                                as._groupName, as._querier,
                                                as._queryType, as._queryQualifiers,
                                                as._query);
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedArrivedSearchReply) {
                        QueuedDSProProxy.QueuedArrivedSearchReply asr = (QueuedDSProProxy.QueuedArrivedSearchReply) cback;
                        DSProProxy.searchReplyArrived (_searchListeners, asr._queryId,
                                                asr._matchingIds, asr._responderNodeId);
                    }
                    else if (cback instanceof QueuedDSProProxy.QueuedArrivedVolatileSearchReply) {
                        QueuedDSProProxy.QueuedArrivedVolatileSearchReply avsr = (QueuedDSProProxy.QueuedArrivedVolatileSearchReply) cback;
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

    public void requestTermination()
    {
        _terminate.set(true);
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
            proxy.registerDSProProxyListener(this);
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
            proxy.deregisterDSProProxyListener(this);
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
            proxy.registerSearchListener (this);
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
            proxy.deregisterSearchListener (this);
        }
    }

    private final List<DSProProxyListener> _dsProListeners = new ArrayList<DSProProxyListener>();
    private final List<SearchListener> _searchListeners = new ArrayList<SearchListener>();

    protected final static Logger LOG = LogUtils.getLogger (AsyncDSProProxy.class);
}
