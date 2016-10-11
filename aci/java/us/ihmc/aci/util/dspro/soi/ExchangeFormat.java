package us.ihmc.aci.util.dspro.soi;

/**
 * ExchangeFormat.java
 * <p/>
 * Base class containing fields parsed from generic <code>InformationObject</code> instances.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
abstract public class ExchangeFormat
{
    public static final String DOMAIN_DATA_TYPE_NAME = "domainData.typeName";
    public static final String INFORMATION_OBJECT_TYPE_NAME = "informationObject.typeName";

    //resource
    public static final String RESOURCE_INSTANCE_ID = "resource.instanceId";
    public static final String RESOURCE_OBJECT_ID = "resource.objectId";
    public static final String RESOURCE_MESSAGE_TYPE = "resource.messageType"; //CUSTOM PROPERTY
    public static final String RESOURCE_CLASSIFICATION = "resource.classification";
    public static final String RESOURCE_OWNER_PRODUCER = "resource.ownerProducer";
    public static final String RESOURCE_DATE_CREATED = "resource.dateCreated";
    public static final String RESOURCE_DATE_POSTED = "resource.datePosted";
    public static final String RESOURCE_CREATOR_SERVICE_NAME = "resource.creatorServiceName";
    public static final String RESOURCE_PUBLISHER_SERVICE_NAME = "resource.publisherServiceName";
    public static final String RESOURCE_FORMAT_MEDIA_MIME_TYPE = "resource.formatMediaMimeType";
    public static final String RESOURCE_SUBJECT_COVERAGE_KEYWORD_VALUE = "subject.coverage.keyword.value";

    //extra dspro values
    //these values aren't parsed but added later to the message
    public static final String DSPRO_ACTION = "dspro.trackAction";
    public static final String DSPRO_PREVIOUS_INSERT_MSG_ID = "dspro.previousInsertMessageID";
    public static final String DSPRO_PREVIOUS_DELETE_MSG_ID = "dspro.previousDeleteMessageID";
    public static final String DSPRO_SEQUENCE_ID = "dspro.sequenceID";
    public static final String DSPRO_SESSION_ID = "dspro.sessionID";
    public static final String SOI_MESSAGE = "soi.message";

    // dspro values used to fill metadata added to the SortedProperties
    public static final String DESCRIPTION = "description";
    public static final String DATA_CONTENT = "data.content";
    public static final String DATA_FORMAT = "data.format";
    public static final String TARGET_IDS_COUNT = "recipients.username.count";
    public static final String TARGET_ID = "recipients.username.number.";
    public static final String NODE_TYPE = "node.type";
    public static final String IMPORTANCE = "importance";
    public static final String LOCATION_LATITUDE = "location.latitude";
    public static final String LOCATION_LONGITUDE = "location.longitude";
    public static final String BAS64_ENCODED_IMAGES = "base64.encoded.images";
    public static final String REMOVE_METADATA_FROM_AM = "remove.metadata.from.application.metadata";

    public static enum Action
    {
        Insert, Delete, KeepAlive
    }
}
