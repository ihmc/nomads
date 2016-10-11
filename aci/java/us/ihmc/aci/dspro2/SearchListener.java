/*
 * SUDADSProProxy.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import java.util.Collection;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public interface SearchListener
{
    public void searchArrived (String queryId, String groupName, String querier,
                               String queryType, String queryQualifiers, byte[] query);
    public void searchReplyArrived (String queryId, Collection<String> matchingMessageIds, String responderNodeId);
    public void searchReplyArrived (String queryId, byte[] reply, String responderNodeId);
}

