/*
 * CtrlMsgListener.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public interface CtrlMsgListener
{
    public void contextUpdateMessageArrived (String senderNodeId, String publisherNodeId);
    public void contextVersionMessageArrived (String senderNodeId, String publisherNodeId);
    public void messageRequestMessageArrived (String senderNodeId, String publisherNodeId);
    public void chunkRequestMessageArrived (String senderNodeId, String publisherNodeId);
    public void positionMessageArrived (String senderNodeId, String publisherNodeId);
    public void searchMessageArrived (String senderNodeId, String publisherNodeId);
    public void topologyReplyMessageArrived (String senderNodeId, String publisherNodeId);
    public void topologyRequestMessageArrived (String senderNodeId, String publisherNodeId);
    public void updateMessageArrived (String senderNodeId, String publisherNodeId);
    public void versionMessageArrived (String senderNodeId, String publisherNodeId);
    public void waypointMessageArrived (String senderNodeId, String publisherNodeId);
    public void wholeMessageArrived (String senderNodeId, String publisherNodeId);
}
