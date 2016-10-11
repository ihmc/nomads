package us.ihmc.aci.util.dspro.soi;


/**
 * SigactExchangeFormat.java
 * <p/>
 * Class containing fields parsed from <code>InformationObject</code> instances of <code>SigAct</code> type.
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class SigactExchangeFormat extends ImagesExchangeFormat
{
    public static final String SIGACT_NAME = "sigact.name";

    public static final String ORIGINATOR = "originator";
    public static final String DATE_TIME_CREATED = "date.time.created";
    public static final String SECURITY_CLASSIFICATION_CLASS_LEVEL = "security.classification.class.level";
    public static final String SECURITY_CLASSIFICATION_CONTROL_MARK = "security.classification.control.mark";
    public static final String SECURITY_CLASSIFICATION_RELEASE_MARK = "security.classification.release.mark";
    public static final String SECURITY_DECLASSIFICATION_SOURCE = "security.declassification.source";
    public static final String SECURITY_DECLASSIFICATION_DATE_TIME = "security.declassification.date.time";
    public static final String SECURITY_DECLASSIFICATION_INSTRUCTIONS = "security.declassification.instructions";
    public static final String SECURITY_DECLASSIFICATION_EXEMPION_CODE = "security.declassification.exempion.code";

    public static final String ENTITY_ID = "entity.id";
    public static final String UNIT_IDENTIFICATION = "unit.identification";
    public static final String DATE_TIME_LAST_MOFIDIED = "date.time.last.modified";

    public static final String CREATION_LATITUDE = "creation.latitude";
    public static final String CREATION_LONGITUDE = "creation.longitude";

    public static final String LOCATION_LATITUDE = "location.latitude";
    public static final String LOCATION_LONGITUDE = "location.longitude";
    public static final String LOCATION_ALTITUDE = "location.altitude";
    public static final String LOCATION_ELEVATION = "location.elevation";

    public static final String COURSE = "course";
    public static final String SPEED = "speed";
    public static final String SYMBOL_CODE = "symbol_code";

    public static final String REMARKS_NUMBER = "remarks.count";
    public static final String REMARKS_SUBJECT_ELEMENT = "remark.subject.number.";
    public static final String REMARKS_DESCRIPTION_ELEMENT = "remark.description.number.";

    public static final String EVENT_NAME = "event.name";

    public static final String DATE_TIME_BEGIN = "date.time.begin";
    public static final String DATE_TIME_END = "date.time.end";
    public static final String OPERATION = "operation";
    public static final String EVENT_DESCRIPTION = "event.description";
    public static final String EVENT_TYPE = "event.type";
    public static final String N_PARTICIPANTS = "n.participants";

//    public static final String DSPRO_SIGACTS_DESCRIPTION = "SIGACT";    // Moved to DSProMimeType
//    public static final String DSPRO_SIGACTS_MIME_TYPE = "x-dspro/x-soi-sigact";    // Moved to DSProMimeType
}
