/*
 * DisseminationServiceProxyListener.java
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

/**
 * Date: May 30, 2008
 * Time: 5:26:58 PM
 *
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision: 1.23 $
 */
public interface DisseminationServiceProxyListener
{
   
    /**
     * Callback function that is invoked when new data arrives.
     *
     * @param msgId
     * @param sender - id of the sender.
     * @param groupName - the group to which the message was addressed.
     * @param seqNum - id of the message.
     * @param objectId
     * @param instanceId
     * @param mimeType
     * @param data - the data that was received.
     * @param metadataLength - the length of the metadata data in bytes. It means that the first
     *                         metadataLength bytes of metadataLength.length are actually metadata
     * @param tag - the tag that was specified for the message
     * @param priority- the priority value that was specified for the message
     * @param queryId
     */
    public void dataArrived (String msgId, String sender, String groupName, int seqNum,
                             String objectId, String instanceId, String mimeType, byte[] data,
                             int metadataLength, short tag, byte priority, String queryId);

    /**
     * Callback function that is invoked when a new chunk (a part of a larger
     * message that may be interpreted without the rest of the message) arrives.
     *
     * @param msgId
     * @param sender - id of the sender.
     * @param groupName - the group to which the message was addressed.
     * @param seqNum - id of the message.
     * @param objectId
     * @param instanceId
     * @param mimeType
     * @param data - the data that was received.
     * @param nChunks - the number of chunks that have been received for the message.
     * @param totNChunks - the number of chunks that the data was fragmented.
     * @param chunkedMsgId
     * @param tag - the tag that was specified for the message
     * @param priority- the priority value that was specified for the message
     * @param queryId
     */
    public void chunkArrived (String msgId, String sender, String groupName, int seqNum,
                              String objectId, String instanceId, String mimeType,
                              byte[] data, short nChunks, short totNChunks,
                              String chunkedMsgId, short tag, byte priority, String queryId);

    /**
     * Callback function that is invoked when new metadata arrives.
     *
     * @param msgId
     * @param sender - id of the sender.
     * @param groupName - the group to which the message was addressed.
     * @param seqNum - id of the message.
     * @param objectId
     * @param instanceId
     * @param dataMimeType
     * @param metadata - the metadata that was received.
     * @param dataChunked
     * @param tag - the tag that was specified for the message
     * @param priority- the priority value that was specified for the message
     * @param queryId
     */
    public void metadataArrived (String msgId, String sender, String groupName, int seqNum,
                                 String objectId, String instanceId, String dataMimeType,
                                 byte[] metadata, boolean dataChunked, short tag,
                                 byte priority, String queryId);

    /**
     * Callback function that is invoked when new metadata describing an available
     * data arrives.
     * The actual data - described by the metadata arrived - is available to be
     * retrieved.
     *
     * @param msgId 
     * @param sender - id of the sender.
     * @param groupName - the group to which the message was addressed.
     * @param seqNum - id of the message.
     * @param objectId
     * @param instanceId
     * @param mimeType
     * @param id - the id of the message that has become available
     * @param metadata - the metadata describing the message.
     * @param tag - the tag that was specified for the message
     * @param priority- the priority value that was specified for the message
     * @param queryId
     */
    public void dataAvailable (String msgId, String sender, String groupName, int seqNum, String objectId,
                               String instanceId, String mimeType, String id, byte[] metadata,
                               short tag, byte priority, String queryId);
}

