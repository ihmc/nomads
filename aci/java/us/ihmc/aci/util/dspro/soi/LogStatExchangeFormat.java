package us.ihmc.aci.util.dspro.soi;

/**
 * LogStatExchangeFormat.java
 * <p/>
 * Class containing fields parsed from <code>InformationObject</code> instances of Log Stat GHub data type.
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class LogStatExchangeFormat extends ARLExchangeFormat
{
    public static final String ID = "id";
    public static final String CREATED_DATE_TIME = "created.date.time";
    public static final String SUBMITTED_DATE_TIME = "submitted.date.time";
    public static final String UNIT_NAME = "unit.name";
    public static final String UIC = "uic";
    public static final String USER = "user";

    public static final String CHILD_REPORTS_NUMBER = "child.reports.count";
    public static final String CHILD_REPORT_ID = "child.report.id.number.";
    public static final String CHILD_REPORT_UIC = "child.report.uic.number.";

    public static final String OVERALL_STATUS = "overall.status";
    public static final String OVERALL_STATUS_NOTE = "overall.status.note";

    public static final String PERSONNEL_STATUS_MARINE_OFFICER = "personnel.status.marine.officer";
    public static final String PERSONNEL_STATUS_MARINE_ENLISTED = "personnel.status.marine.enlisted";
    public static final String PERSONNEL_STATUS_NAVY_OFFICER = "personnel.status.navy.officer";
    public static final String PERSONNEL_STATUS_NAVY_ENLISTED = "personnel.status.navy.enlisted";
    public static final String PERSONNEL_STATUS_OTHER = "personnel.status.other";
    public static final String PERSONNEL_STATUS_NOTE = "personnel.status.note";

    public static final String C1_SUPPLIES = "class.one.supplies";
    public static final String C1_SUPPLIES_STATUS = "class.one.supplies.status";
    public static final String C1_SUPPLIES_NOTE = "class.one.supplies.note";
    public static final String C1_SUPPLIES_ITEMS_NUMBER = "class.one.supplies.items.count";
    public static final String C1_SUPPLIES_ITEM_NSN = "class.one.supplies.item.nsn.number.";
    public static final String C1_SUPPLIES_ITEM_NOMENCLATURE = "class.one.supplies.item.nomenclature.number.";
    public static final String C1_SUPPLIES_ITEM_DAILY_RATE = "class.one.supplies.item.daily.rate.number.";
    public static final String C1_SUPPLIES_ITEM_STOCKING_OBJECTIVE = "class.one.supplies.item.stocking.objective.number.";
    public static final String C1_SUPPLIES_ITEM_ON_HAND = "class.one.supplies.item.on.hand.number.";
    public static final String C1_SUPPLIES_ITEM_DOS_ON_HAND = "class.one.supplies.item.dos.on.hand.number.";

    public static final String C3_SUPPLIES = "class.three.supplies";
    public static final String C3_SUPPLIES_STATUS = "class.three.supplies.status";
    public static final String C3_SUPPLIES_NOTE = "class.three.supplies.note";
    public static final String C3_SUPPLIES_ITEMS_NUMBER = "class.three.supplies.items.count";
    public static final String C3_SUPPLIES_ITEM_NSN = "class.three.supplies.item.nsn.number.";
    public static final String C3_SUPPLIES_ITEM_NOMENCLATURE = "class.three.supplies.item.nomenclature.number.";
    public static final String C3_SUPPLIES_ITEM_DAILY_RATE = "class.three.supplies.item.daily.rate.number.";
    public static final String C3_SUPPLIES_ITEM_STOCKING_OBJECTIVE = "class.three.supplies.item.stocking.objective.number.";
    public static final String C3_SUPPLIES_ITEM_ON_HAND = "class.three.supplies.item.on.hand.number.";
    public static final String C3_SUPPLIES_ITEM_DOS_ON_HAND = "class.three.supplies.item.dos.on.hand.number.";

    public static final String C5_SUPPLIES = "class.five.supplies";
    public static final String C5_SUPPLIES_STATUS = "class.five.supplies.status";
    public static final String C5_SUPPLIES_NOTE = "class.five.supplies.note";
    public static final String C5_SUPPLIES_ITEMS_NUMBER = "class.five.supplies.items.count";
    public static final String C5_SUPPLIES_ITEM_NSN = "class.five.supplies.item.nsn.number.";
    public static final String C5_SUPPLIES_ITEM_DODIC = "class.five.supplies.item.dodic.number.";
    public static final String C5_SUPPLIES_ITEM_NOMENCLATURE = "class.five.supplies.item.nomenclature.number.";
    public static final String C5_SUPPLIES_ITEM_STOCKING_OBJECTIVE = "class.five.supplies.item.stocking.objective.number.";
    public static final String C5_SUPPLIES_ITEM_ON_HAND = "class.five.supplies.item.on.hand.number.";

    public static final String C7_SUPPLIES = "class.seven.supplies";
    public static final String C7_SUPPLIES_STATUS = "class.seven.supplies.status";
    public static final String C7_SUPPLIES_NOTE = "class.seven.supplies.note";
    public static final String C7_SUPPLIES_ITEMS_NUMBER = "class.seven.supplies.items.count";
    public static final String C7_SUPPLIES_ITEM_TAMCN = "class.seven.supplies.item.tamcn.number.";
    public static final String C7_SUPPLIES_ITEM_NOMENCLATURE = "class.seven.supplies.item.nomenclature.number.";
    public static final String C7_SUPPLIES_ITEM_ON_HAND = "class.seven.supplies.item.on.hand.number.";
    public static final String C7_SUPPLIES_ITEM_READINESS = "class.seven.supplies.item.readiness.number.";
    public static final String C7_SUPPLIES_ITEM = "class.seven.supplies.item.number.";
    public static final String C7_DEADLINED_ITEMS_NUMBER = ".deadlined.items.count";     // To be used as C7_SUPPLIES_ITEM + i + C7_DEADLINED_ITEMS_NUMBER
    public static final String C7_DEADLINED_ITEM_NIIN = ".deadlined.item.niin.";         // To be used as C7_SUPPLIES_ITEM + i + C7_DEADLINED_ITEM_NIIN + j
    public static final String C7_DEADLINED_ITEM_SERIAL = ".deadlined.item.serial.";     // To be used as C7_SUPPLIES_ITEM + i + C7_DEADLINED_ITEM_SERIAL + j

//    public static final String DSPRO_DESCRIPTION = "LogStat";   // moved to DSProMimeType
    //public static final String DSPRO_GHUB_MIME_TYPE = "x-dspro/x-soi-ghub-log-stat";
    //public static final String DSPRO_MIME_TYPE = "x-dspro/x-soi-logstat";   // moved to DSProMimeType
}
