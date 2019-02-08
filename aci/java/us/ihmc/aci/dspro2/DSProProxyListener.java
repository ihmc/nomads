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
    boolean pathRegistered (NodePath path, String nodeId, String teamId, String mission);
    
    boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId);

    void newNeighbor (String peerID);
    void deadNeighbor (String peerID);

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
    boolean dataArrived (String dsproId, String groupName, String objectId, String instanceId,
                         String annotatedObjMsgId, String mimeType, byte[] data, short chunkNumber,
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
     * @param jsonMetadata - the JSON metadata.
     * @param referredDataDsproId - referred data id of the message.
     * @return
     */
    boolean metadataArrived (String dsproId, String groupName, String referredDataObjectId,
                             String referredDataInstanceId, String jsonMetadata,
                             String referredDataDsproId, String callbackParameters);

    boolean dataAvailable (String id, String groupName, String objectId, String instanceId,
                           String referredDataId, String mimeType, byte[] metadata, String queryId);
}
