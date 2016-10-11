package us.ihmc.aci.util.dspro.soi;

/**
 * Class containing fields parsed from <code>InformationObject</code> instances of <code>RapidRequest</code> type
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/21/2016
 */
public class RapidRequestExchangeFormat extends ARLExchangeFormat
{
    public static final String CORRELATION_ID = "correlationId";
    public static final String SYSTEM_ID = "systemId";
    public static final String URGENCY = "urgency";
    public static final String TO_SYSTEM = "toSystem";
    public static final String SUPPORTING_UNIT_UIC = "supportingUnitUic";
    public static final String COMMENTS = "comments";
    public static final String REQUEST_DELIVERY_DATE = "requestDeliveryDate";
    public static final String DELIVERY_LATITUDE = "deliveryLat";
    public static final String DELIVERY_LONGITUDE = "deliveryLon";
    public static final String SUPPLY_ITEMS_COUNT = "supplyItemsCount";
    public static final String SUPPLY_ITEM_COS = "supplyItem.cos.";
    public static final String SUPPLY_ITEM_QUANTITY = "supplyItem.quantity.";
    public static final String SUPPLY_ITEM_UOI = "supplyItem.uoi.";
    public static final String SUPPLY_ITEM_IDENT_TYPE = "supplyItem.identType.";
    public static final String SUPPLY_ITEM_IDENTIFIER = "supplyItem.identifier.";
    public static final String LOG_SERVICE_ITEMS_COUNT = "logServiceItemsCount";
    public static final String LOG_SERVICE_ITEM_REQUEST = "logServiceItem.request.";
    public static final String LOG_SERVICE_ITEM_TASK_TYPE = "logServiceItem.taskType.";
}
