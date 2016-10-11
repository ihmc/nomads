/*
 * DSProProxyListener.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import us.ihmc.aci.util.dspro.NodePath;

/**
 * @author Maggie Breedy    (mbreedy@ihmc.us)
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public interface DSProProxyListener
{
    public boolean pathRegistered (NodePath path, String nodeId, String teamId, String mission);
    
    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId);

    public void newNeighbor (String peerID);
    public void deadNeighbor (String peerID);

     /**
     * Callback function that is invoked when new data arrives.
     *
     * @param dsproId - id of the arrived metadata message.
     * @param groupName - the name of the group the metadata message belongs to.
     * @param objectId
     * @param instanceId
     * @param annotatedObjMsgId
     * @param mimeType
     * @param data - the data that was received.
     * @param chunkNumber - the number of chunks.
     * @param totChunksNumber - the total number of chunks
     * @param callbackParameters
     * @return
     */
    public boolean dataArrived (String dsproId, String groupName, String objectId,
                                String instanceId, String annotatedObjMsgId,
                                String mimeType, byte[] data, short chunkNumber,
                                short totChunksNumber, String callbackParameters);

    /**
     * Callback function that is invoked when new metadata arrives.
     *
     * @param dsproId - id of the arrived metadata message.
     * @param groupName
     * @param referredDataObjectId
     * @param referredDataInstanceId
     * @param callbackParameters
     * @groupName - the name of the group the metadata message belongs to.
     * @param XMLMetadata - the XML metadata.
     * @param referredDataDsproId - referred data id of the message.
     * @return
     */
    public boolean metadataArrived (String dsproId, String groupName, String referredDataObjectId,
                                    String referredDataInstanceId, String XMLMetadata,
                                    String referredDataDsproId, String callbackParameters);
}
