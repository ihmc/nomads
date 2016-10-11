package us.ihmc.aci.util.dspro.soi;

/**
 * Class containing fields parsed from <code>InformationObject</code> instances of general ARL data types
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class ARLExchangeFormat extends ExchangeFormat
{
    public static final String INR_ID = "inr.id";
    public static final String INF_ID = "inf.id";

    public static final String RECIPIENTS_COUNT = "recipients.count";
    public static final String RECIPIENTS_ALERT_PRIORITY = "recipients.missionAlert.priority.number.";
    public static final String RECIPIENTS_CHANGE_PRIORITY = "recipients.change.priority.number.";
    public static final String RECIPIENTS_INITIAL_PRIORITY = "recipients.initial.priority.number.";
    public static final String RECIPIENTS_BILLET = "recipients.billet.number.";
    public static final String RECIPIENTS_ROLE = "recipients.role.number.";
    public static final String RECIPIENTS_UIC = "recipients.uic.number.";
    public static final String RECIPIENTS_FUNCTION = "recipients.function.number.";

    public static final String RECIPIENTS_USERNAME_COUNT = ExchangeFormat.TARGET_IDS_COUNT; // Do not change this
    public static final String RECIPIENTS_USERNAME = ExchangeFormat.TARGET_ID;  // Do not change this

    public static final String ARL_OBJECT_TYPE = "arl.object.type";

    public static final String NAME = "ghub.data.name";
    public static final String MIL_STD_2525_SYMBOL_ID = "milStd2525SymbolId";
    public static final String LOCATION_LATITUDE = "latitude";
    public static final String LOCATION_LONGITUDE = "longitude";

    public static final String IMAGES_URL_NUMBER = ImagesExchangeFormat.IMAGES_URL_NUMBER;

    public static final String PRIORITY_LEVEL = "priority.level";

    public static final String TIMESTAMP = "timestamp";
    public static final String UIC = "uic";
}
