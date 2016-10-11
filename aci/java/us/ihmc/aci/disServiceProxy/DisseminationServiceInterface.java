/*
 * DisseminationServiceInterface.java
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
import java.util.List;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public interface DisseminationServiceInterface extends DisseminationServiceProxyListener, PeerStatusListener, SearchListener
{
    public int init() throws Exception;
    public void reinitialize();
    public boolean isInitialized();
    public String getNodeId() throws CommException;
    public List<String> getPeerList();
    public String store (String groupName, String objectId, String instanceId,
                         String mimeType, byte[] metaData, byte[] data, long expiration,
                         short historyWindow, short tag, byte priority)
        throws CommException;
    public void push (String msgId) throws CommException;
    public String push (String groupName, String objectId, String instanceId,
                        String mimeType, byte[] metaData, byte[] data, long expiration,
                        short historyWindow, short tag, byte priority)
        throws CommException;
    public String makeAvailable (String groupName, String objectId, String instanceId,
                                 byte[] metadata, byte[] data, String dataMimeType,
                                 long expiration, short historyWindow, short tag, byte priority)
        throws CommException;
    public void cancel (String id) throws CommException, ProtocolException;
    public void cancel (short tag) throws CommException, ProtocolException;
    public boolean addFilter (String groupName, short tag) throws CommException;
    public boolean removeFilter (String groupName, short tag) throws CommException;
    public boolean requestMoreChunks (String groupName, String senderNodeId, int seqId) throws CommException;
    public boolean requestMoreChunks (String messageId) throws CommException;
    public byte[] retrieve (String id, int timeout) throws CommException;
    public int retrieve (String id, String filePath);
    public boolean request (String groupName, short tag, short historyLength, long timeout) throws CommException;
    public String search (String groupName, String queryType, String queryQualifiers, byte[] query) throws CommException;
    public void replyToSearch (String queryId, Collection<String> disServiceMsgIds) throws CommException;
    public boolean subscribe (String groupName, byte priority, boolean groupReliable,
                              boolean msgReliable, boolean sequenced) throws CommException;
    public boolean subscribe (String groupName, short tag, byte priority,
                              boolean groupReliable, boolean msgReliable, boolean sequenced) throws CommException;
    public boolean subscribe (String groupName, byte predicateType, String predicate, byte priority,
                              boolean groupReliable, boolean msgReliable, boolean sequenced) throws CommException;
    public boolean unsubscribe (String groupName) throws CommException;
    public boolean unsubscribe (String groupName, short tag) throws CommException;
    public void registerDisseminationServiceProxyListener (DisseminationServiceProxyListener listener) throws CommException;
    public void registerDisseminationServiceProxyListener (short clientId, DisseminationServiceProxyListener listener)
            throws ListenerAlreadyRegisteredException;
    public void registerPeerStatusListener (PeerStatusListener listener);
    public void registerSearchListener (SearchListener listener);
    public void registerConnectionStatusListener (ConnectionStatusListener listener);
    public boolean resetTransmissionHistory() throws CommException;
}
