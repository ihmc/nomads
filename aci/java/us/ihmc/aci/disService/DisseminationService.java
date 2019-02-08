/*
 * DisseminationService.java
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

package us.ihmc.aci.disService;

import java.util.List;

/**
 * Dissemination Service Java JNI Wrapper
 *
 * Date: May 30, 2008
 * Time: 3:24:57 PM
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 *
 */
public class DisseminationService
{
    public DisseminationService()
    {
        init (null);
    }

    public DisseminationService (String configFile)
    {
        init (configFile);
    }

    public native String getNodeId();

    public native List<String> getPeerList();

    public native synchronized String push (String groupName, String objectId, String instanceId, String mimeType, byte[] metaData,
                                            byte[] data, long expiration, short historyWindow, short tag, byte priority);

    public native synchronized String makeAvailable (String groupName, String objectId, String instanceId, byte[] metadata, byte[] data,
                                                     String dataMimeType, long expiration, short historyWindow, short tag, byte priority);

    public native synchronized void cancel (String id);

    public native synchronized void cancel (short tag);

    public native synchronized boolean addFilter (String groupName, short tag);

    public native synchronized boolean removeFilter (String groupName, short tag);

    public native synchronized boolean requestMoreChunks (String groupName, String senderNodeId, int seqId);

    public native synchronized boolean requestMoreChunks (String messageId);

    public native synchronized byte[] retrieve (String id, int timeout);

    public native synchronized int retrieve (String id, String filePath);

    public native synchronized boolean historyRequest (String groupName, short tag, short historyLength, long timeout);

    public native synchronized boolean subscribe (String groupName, byte priority, boolean groupReliable,
                                                  boolean msgReliable, boolean sequenced);

    public native synchronized boolean subscribe (String groupName, short tag, byte priority,
                                                  boolean groupReliable, boolean msgReliable, boolean sequenced);

    public native synchronized boolean subscribe (String groupName, byte predicateType, String predicate, byte priority,
                                                  boolean groupReliable, boolean msgReliable, boolean sequenced);

    public native synchronized boolean unsubscribe (String groupName);

    public native synchronized boolean unsubscribe (String groupName, short tag);

    public native void registerDisseminationServiceListener (DisseminationServiceListener listener);

    public native void registerPeerStatusListener (PeerStatusListener listener);
	
    public native boolean resetTransmissionHistory();

    public native synchronized String getNextPushId (String groupName);

    // /////////////////////////////////////////////////////////////////////////
    private native void init (String configFile);

    private long _disService;    
    // /////////////////////////////////////////////////////////////////////////
    static
    {
        System.loadLibrary ("DisServiceJNIWrapper");
    }
}
