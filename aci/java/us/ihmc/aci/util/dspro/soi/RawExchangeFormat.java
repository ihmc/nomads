package us.ihmc.aci.util.dspro.soi;

/**
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class RawExchangeFormat extends ExchangeFormat
{
//    public static final String DSPRO_DESCRIPTION = "raw-SOI-data";  // Moved to DSProMimeType
    public static final String TARGET_IDS_COUNT = "recipients.username.count";
    public static final String TARGET_ID = "recipients.username.number.";
    public static final String IMPORTANCE = "importance";
//    public static final String DSPRO_RAW_DATA_MIME_TYPE = "x-dspro/x-soi-raw-data"; // Moved to DSProMimeType
    public static final String SOI_MESSAGE = ExchangeFormat.SOI_MESSAGE;
}
