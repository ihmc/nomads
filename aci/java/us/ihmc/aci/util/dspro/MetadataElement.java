package us.ihmc.aci.util.dspro;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public enum MetadataElement {

    applicationMetadata,
    applicationMetadataFormat,
    applicationId,
    messageId,

    refersTo,

    referredDataObjectId,
    referredDataInstanceId,

    externalReferredCachedDataURL,

    computedVoi,

    prevMsgId,
    nodeType,
    dataName,
    classification,
    dataFormat,
    leftUpperLatitude,
    rightLowerLatitude,
    leftUpperLongitude,
    rightLowerLongitude,
    description,
    pedigree,
    importance,
    location,
    receiverTimestamp,
    source,
    sourceReliability,
    sourceTimestamp,
    expirationTime,
    relevantMissions,
    targetId,
    targetRole,
    targetTeam,
    trackId,
    trackAction,
    abeEquation,
    abeIV,
    abeEncryptedFields;
}
