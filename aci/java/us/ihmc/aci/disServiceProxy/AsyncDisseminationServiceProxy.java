/*
 * AsyncDisseminationServiceProxy.java
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

import java.util.ArrayList;
import java.util.Collection;

import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class AsyncDisseminationServiceProxy extends QueuedDisseminationServiceProxy implements Runnable
{
    public static final long DEFAULT_POLLING_TIME = 5000;

    private final long _pollingTime;

    public AsyncDisseminationServiceProxy()
    {
        this (DEFAULT_POLLING_TIME);
    }

    public AsyncDisseminationServiceProxy (long pollingTime)
    {
        super();
        _pollingTime = pollingTime;
    }

    public AsyncDisseminationServiceProxy (short applicationId)
    {
        this (applicationId, DEFAULT_POLLING_TIME);
    }

    public AsyncDisseminationServiceProxy (short applicationId, long pollingTime)
    {
        super (applicationId);
        _pollingTime = pollingTime;
    }

    public AsyncDisseminationServiceProxy (short applicationId, long reinitializationAttemptInterval, long pollingTime)
    {
        super (applicationId, reinitializationAttemptInterval);
        _pollingTime = pollingTime;
    }

    public AsyncDisseminationServiceProxy (short applicationId, String host, int iPort)
    {
    	this (applicationId, host, iPort, DEFAULT_POLLING_TIME);
    }

    public AsyncDisseminationServiceProxy (short applicationId, String host, int iPort, long pollingTime)
    {
    	super (applicationId, host, iPort);
        _pollingTime = pollingTime;
    }

    public AsyncDisseminationServiceProxy (short applicationId, String host, int port,
                                           long reinitializationAttemptInterval, long pollingTime)
    {
        super (applicationId, host, port, reinitializationAttemptInterval);
        _pollingTime = pollingTime;
    }

    public void run()
    {
        while (true) {
            try {
                boolean atLeastOne = false;

                QueuedArrivedData ad = getArrivedData();
                if (ad != null) {
                    DisseminationServiceProxy.dataArrived (_listeners, ad.msgId, ad.sender, ad.groupName, ad.seqNum,
                                                         ad.objectId, ad.instanceId, ad.mimeType,
                                                         ad.data, ad.metadataLength,
                                                         ad.tag, ad.priority, ad.queryId);
                    atLeastOne = true;
                }

                QueuedArrivedChunk ac = getArrivedChunk();
                if (ac != null) {
                    DisseminationServiceProxy.chunkArrived (_listeners, ac.msgId, ac.sender, ac.groupName, ac.seqNum,
                                                          ac.objectId, ac.instanceId, ac.mimeType, ac.data,
                                                          ac.nChunks, ac.totNChunks, ac.chunhkedMsgId,
                                                          ac.tag, ac.priority, ac.queryId);
                    atLeastOne = true;
                }

                QueuedArrivedMetadata am = getArrivedMetadata();
                if (am != null) {                
                    DisseminationServiceProxy.metadataArrived (_listeners, am.msgId, am.sender, am.groupName, am.seqNum,
                                                             am.objectId, am.instanceId, am.mimeType,
                                                             am.data, am.chunkedData, am.tag,
                                                             am.priority, am.queryId);
                    atLeastOne = true;
                }

                QueuedAvailableData vd = getAvailableData();
                if (vd != null) {                
                    DisseminationServiceProxy.dataAvailable (_listeners, vd.msgId, vd.sender, vd.groupName, vd.seqNum,
                                                           vd.objectId, vd.instanceId, vd.mimeType,
                                                           vd.refObjId, vd.data, vd.tag, vd.priority,
                                                           vd.queryId);
                    atLeastOne = true;
                }

                PeerListUpdate update = getPeerListUpdate();
                if (update != null) {
                    for (String peerId : update._newPeersList) {
                        DisseminationServiceProxy.newPeer (_peerStatusListeners, peerId);
                    }
                    for (String peerId : update._deadPeersList) {
                        DisseminationServiceProxy.deadPeer (_peerStatusListeners, peerId);
                    }
                    atLeastOne = true;
                }

                ArrivedSearch as = getArrivedSearch();
                if (as != null) {                
                    DisseminationServiceProxy.searchArrived (_searchListeners, as._queryId, as._groupName,
                                                           as._querier, as._queryType, as._queryQualifiers,
                                                           as._query);
                    atLeastOne = true;
                }

                ArrivedSearchReply asr = getArrivedSearchReply();
                if (as != null) {                
                    DisseminationServiceProxy.searchReplyArrived (_searchListeners, asr._queryId, asr._matchingIds,
                                                           asr._responderNodeId);
                    atLeastOne = true;
                }

                if (!atLeastOne) {
                    waitIfEmpty();
                }
            }
            catch (Exception e) {
                // Just to make sure that the method does not end because the
                // notified application is not handling an exception...
            }
        }
    }

    @Override
    public void registerDisseminationServiceProxyListener(DisseminationServiceProxyListener listener)
        throws CommException
    {
        if (listener == null) {
            System.out.println ("Error:DisseminationServiceProxyListener is null");
            return;
        }
        if (_listeners.isEmpty()) {
            super.registerDisseminationServiceProxyListener (this);
        }
        _listeners.add (listener);
    }

    @Override
    public void registerPeerStatusListener (PeerStatusListener listener)
    {
        if (listener == null) {
            System.out.println ("Error:PeerStatusListener is null");
            return;
        }
        if (_peerStatusListeners.isEmpty()) {
            super.registerPeerStatusListener (this);
        }
        _peerStatusListeners.add (listener);
    }

    @Override
    public void registerSearchListener (SearchListener listener)
    {
        if (listener == null) {
            System.out.println ("Error:SearchListeneris null");
            return;
        }
        if (_searchListeners.isEmpty()) {
            super.registerSearchListener (this);
        }
        _searchListeners.add (listener);
    }

    private final Collection<DisseminationServiceProxyListener> _listeners = new ArrayList<DisseminationServiceProxyListener>();
    private final Collection<PeerStatusListener> _peerStatusListeners = new ArrayList<PeerStatusListener>();
    private final Collection<SearchListener> _searchListeners = new ArrayList<SearchListener>();
}
