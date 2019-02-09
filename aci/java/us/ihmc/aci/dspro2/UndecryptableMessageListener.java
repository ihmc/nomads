package us.ihmc.aci.dspro2;
/**
 * @author Lorenzo Campioni    (lcampioni@ihmc.us)
 * 
 */

public interface UndecryptableMessageListener {
	
    boolean undecryptableDataArrived (String dsproId, String groupName, String objectId,
                                      String instanceId, String annotatedObjMsgId,
                                      String mimeType, byte[] data, short chunkNumber,
                                      short totChunksNumber, String callbackParameters);

    boolean undecryptableMetadataArrived (String dsproId, String groupName, String referredDataObjectId,
                                          String referredDataInstanceId, String xmlMetadata,
                                          String referredDataDsproId, String callbackParameters);
}

