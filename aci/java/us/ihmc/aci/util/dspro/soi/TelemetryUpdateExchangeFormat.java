package us.ihmc.aci.util.dspro.soi;

/**
 * Class containing fields parsed from <code>InformationObject</code> instances of <code>TelemetryUpdate</code> type
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/21/2016
 */
public class TelemetryUpdateExchangeFormat extends TelemetryRequestExchangeFormat
{
    public static final String DESTINATION_DEVICE_ID = "destinationDeviceId";
    public static final String DEVICE_ID = "deviceId";
    public static final String ITEM_ID = "itemId";
    public static final String UPDATE_TYPE = "updateType";
}
