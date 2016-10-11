package us.ihmc.aci.util.dspro.soi;

/**
 * TrackExchangeFormat.java
 * <p/>
 * Class containing fields parsed from <code>InformationObject</code> instances of <code>Track</code> type.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public final class TrackExchangeFormat extends ExchangeFormat
{
    //track
    public static final String TRACK_ENVIRONMENT_CATEGORY = "track.environmentCategory";
    public static final String TRACK_THREAT = "track.threat";
    public static final String TRACK_TACTICAL_TRAINING_CODE = "track.tacticalTrainingCode";
    public static final String TRACK_SCOPE = "track.scope";
    public static final String TRACK_TYPE = "track.type";
    public static final String TRACK_AMBIGUITY_REASON = "track.ambiguityReason";
    public static final String TRACK_UID = "track.UID";
    public static final String TRACK_UNIQUE_REFERENCE_CODE = "track.uniqueReferenceCode";
    public static final String TRACK_COMMAND = "track.command";
    public static final String TRACK_NUMBER = "track.number";
    public static final String TRACK_NICK_NAME = "track.nickName";
    public static final String TRACK_MIL_STD_2525_SYMBOL_ID = "track.milStd2525SymbolId";
    public static final String TRACK_DATA_AMPLIFICATION = "track.dataAmplification";
    public static final String TRACK_ATTACK_MISSION_IN_PROGRESS = "track.attackMissionInProgress";
    public static final String TRACK_ATTACK_MISSION_TASKED = "track.attackMissionTasked";
    public static final String TRACK_BDA_PHASE1_REPORT_AVAILABLE = "track.BDAPhase1ReportAvailable";
    public static final String TRACK_BDA_PHASE2_REPORT_AVAILABLE = "track.BDAPhase2ReportAvailable";
    public static final String TRACK_DMPI_ASSIGNED = "track.DMPIAssigned";
    public static final String TRACK_ENGAGED = "track.engaged";
    public static final String TRACK_NOMINATION_REQUESTED = "track.nominationRequested";
    public static final String TRACK_NOMINATION_REQUEST_REJECTED = "track.nominationRequestRejected";
    public static final String TRACK_ON_ATO = "track.onATO";
    public static final String TRACK_TIME_CRITICAL = "track.timeCritical";
    public static final String TRACK_VALIDATED = "track.validated";
    public static final String TRACK_WEAPON_ASSIGNED = "track.weaponAssigned";
    public static final String TRACK_WEAPONEERED = "track.weaponeered";
    public static final String TRACK_FLAG = "track.flag";
    public static final String TRACK_FORCE_STRUCTURE_ECHELON = "track.forceStructureEchelon";
    public static final String TRACK_FORCE_STRUCTURE_SERVICE = "track.forceStructureService";
    public static final String TRACK_FORCE_STRUCTURE_TASK_ORG_NUMBER = "track.forceStructureTaskOrgNumber";
    public static final String TRACK_NAME = "track.name";
    public static final String TRACK_UNIT_REFERENCE_NUMBER = "track.unitReferenceNumber";
    public static final String TRACK_ORGANIZATION_TYPE = "track.organizationType";
    public static final String TRACK_STRENGTH = "track.strength";

    //event
    public static final String EVENT_GUID = "event.GUID";
    public static final String EVENT_DGT = "event.DGT";
    public static final String EVENT_LATITUDE = "event.latitude";
    public static final String EVENT_LONGITUDE = "event.longitude";
    public static final String EVENT_SPEED = "event.speed";
    public static final String EVENT_SENSOR_CODE = "event.sensorCode";
    public static final String EVENT_SOURCE_CODE = "event.sourceCode";
    public static final String EVENT_SOURCE_REPORT_NUMBER = "event.sourceReportNumber";
    public static final String EVENT_OWNER_ENTITY_UID = "event.ownerEntityUID";

    //extra dspro values
//    public static final String DSPRO_TRACK_DESCRIPTION = "Track";   // Moved to DSProMimeType
//    public static final String DSPRO_TRACK_MIME_TYPE = "x-dspro/x-soi-track"; // Moved to DSProMimeType
    public static final String DSPRO_GROUND_2525_SYMBOL_ID = "SFGP-----------";
    public static final String DSPRO_AIR_2525_SYMBOL_ID = "SFAP-----------";

    public static final float DSPRO_MAX_LATITUDE = 90;
    public static final float DSPRO_MIN_LATITUDE = -90;
    public static final float DSPRO_MAX_LONGITUDE = 180;
    public static final float DSPRO_MIN_LONGITUDE = -180;

    private TrackExchangeFormat ()
    {
        throw new AssertionError();
    }
}
