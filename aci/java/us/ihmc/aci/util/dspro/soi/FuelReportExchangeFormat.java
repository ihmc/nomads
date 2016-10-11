package us.ihmc.aci.util.dspro.soi;

/**
 * Class containing fields parsed from <code>InformationObject</code> instances of <code>FuelReport</code> type
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/21/2016
 */
public class FuelReportExchangeFormat extends ARLExchangeFormat
{
    public static final String SERIAL_NUMBER = "serialNumber";
    public static final String NIIN = "niin";
    public static final String FUEL_TYPE = "fuelType";

    public static final String FTT_TIMESTAMP = "ftt.timestamp";
    public static final String FTT_TRANSACTION_TYPE = "ftt.transactionType";
    public static final String FTT_UIC = "ftt.uic";
    public static final String FTT_METER_ID = "ftt.meterId";
    public static final String FTT_TQ_READING_METHOD = "ftt.tq.readingMethod";
    public static final String FTT_TQ_TEMP = "ftt.tq.temp";
    public static final String FTT_TQ_TEMP_CORRECTED_VALUE = "ftt.tq.tempCorrectedValue";
    public static final String FTT_TQ_UNCORRECTED_VALUE = "ftt.tq.uncorrectedValue";
    public static final String FTT_SM_READING_METHOD = "ftt.sm.readingMethod";
    public static final String FTT_SM_TEMP = "ftt.sm.temp";
    public static final String FTT_SM_TEMP_CORRECTED_VALUE = "ftt.sm.tempCorrectedValue";
    public static final String FTT_SM_UNCORRECTED_VALUE = "ftt.sm.uncorrectedValue";
    public static final String FTT_EM_READING_METHOD = "ftt.em.readingMethod";
    public static final String FTT_EM_TEMP = "ftt.em.temp";
    public static final String FTT_EM_TEMP_CORRECTED_VALUE = "ftt.em.tempCorrectedValue";
    public static final String FTT_EM_UNCORRECTED_VALUE = "ftt.em.uncorrectedValue";
    public static final String FTT_COMMENT = "ftt.comment";
    public static final String FTT_FD_OBSERVED_API = "ftt.fd.observedApi";
    public static final String FTT_FD_OBSERVED_CORRECTED_VOLUME = "ftt.fd.observedCorrectedVolume";
    public static final String FTT_FD_OBSERVED_TEMPERATURE = "ftt.fd.observedTemperature";
    public static final String FTT_FD_WEIGHT = "ftt.fd.weight";
    public static final String FTT_FD_WEIGHT_VOLUME = "ftt.fd.weightVolume";
}
