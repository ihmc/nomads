package us.ihmc.aci.util.dspro.soi;

/**
 * MissionAlertExchangeFormat.java
 * <p/>
 * Class containing fields parsed from <code>InformationObject</code> instances of mission missionAlert data type.
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class MissionAlertExchangeFormat extends ARLExchangeFormat
{
    public static final String MISSION_ALERT_ID = "mission.missionAlert.id";
    public static final String GROUP_ID = "group.id";

    public static final String ALERT_CHANGE_THRESHOLD_CAPTION = "missionAlert.change.threshold.caption";
    public static final String ALERT_CHANGE_THRESHOLD_MESSAGE = "missionAlert.change.threshold.message";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_BOUNDS_REF = "missionAlert.change.threshold.type.bounds.ref";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_COMPAR_OPERATOR = "missionAlert.change.threshold.type.compar.operator";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_DATA = "missionAlert.change.threshold.type.compar.data";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_VALUE = "missionAlert.change.threshold.type.compar.value";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_MEASUREMENT = "missionAlert.change.threshold.type.measurement";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_CAPTION = "missionAlert.change.threshold.type.measurement.caption";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_MESSAGE = "missionAlert.change.threshold.type.measurement.message";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_QUANTITY = "missionAlert.change.threshold.type.quantity";
    public static final String ALERT_CHANGE_THRESHOLD_TYPE_VARIABLE = "missionAlert.change.threshold.type.variable";

    public static final String ALERTING_VALUE = "alerting.value";

    public static final String ALERTING_ENTITY_QUERY_VARIABLE_NAME = "alerting.entity.query.variable.name";
    public static final String ALERTING_ENTITY_QUERY_VARIABLE_DATA_TYPE = "alerting.entity.query.variable.data.type";
    public static final String ALERTING_ENTITY_QUERY_VARIABLE_VALUE = "alerting.entity.query.variable.value";

    public static final String MESSAGE = "message";

    public static final String DSPRO_DATA_XML_MESSAGE = "xml.message.data.content";
//    public static final String DSPRO_DESCRIPTION_HIGHEST_PRIORITY = "MissionAlert-highest-priority";    // Moved to DSProMimeType
//    public static final String DSPRO_DESCRIPTION_MEDIUM_PRIORITY = "MissionAlert-medium-priority";  // Moved to DSProMimeType
//    public static final String DSPRO_DESCRIPTION_LOWEST_PRIORITY = "MissionAlert-lowest-priority";  // Moved to DSProMimeType
//    public static final String DSPRO_GHUB_MIME_TYPE = "x-dspro/x-soi-ghub-mission-missionAlert";
//    public static final String DSPRO_MISSION_ALERT_MIME_TYPE = "x-dspro/x-soi-mission-missionAlert";    // Moved to DSProMimeType
}
