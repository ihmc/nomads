package us.ihmc.aci.util.dspro.soi;

/**
 * Collection of mime type values used in the interface with DSPro
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/22/2016
 */
public enum DSProMimeType
{
    track ("x-dspro/x-soi-track"),
    overlay ("x-dspro/x-soi-overlay"),
    sigact ("x-dspro/x-soi-sigact"),
    missionAlert ("x-dspro/x-soi-mission-missionAlert"),
    dataProduct ("x-dspro/x-soi-data-product"),
    logstat ("x-dspro/x-soi-logstat"),
    infoFulfillment ("x-dspro/x-soi-info-fulfillment"),
    infoRequirement ("x-dspro/x-soi-info-requirement"),
    fuelReport ("x-dspro/x-soi-fuel-report"),
    rapidRequest ("x-dspro/x-soi-rapid-request"),
    telemetryRequest ("x-dspro/x-soi-telemetry-request"),
    telemetryUpdate ("x-dspro/x-soi-telemetry-update"),
    intel ("x-dspro/x-soi-intel-data"),
    jpegImage ("image/jpeg"),
    pngImage ("image/png"),
    bmpImage ("image/bmp"),
    gifImage ("image/gif"),
    raw ("x-dspro/x-soi-raw-data"),
    arbitrary ("x-dspro/arbitrary-soi-message"),

    // SOI Gen 2
    enrichedInformationRequirement ("x-dspro/x-soi-enriched-information-requirement"),
    informationDeficit ("x-dspro/x-soi-information-deficit"),
    informationFulfillmentRegistry ("x-dspro/x-soi-information-fulfillment-registry"),
    informationUnfilled ("x-dspro/x-soi-information-unfilled"),
    intelEvent ("x-dspro/x-soi-intel-event"),
    intelReport ("x-dspro/x-soi-intel-report"),
    mission ("x-dspro/x-soi-mission"),
    product ("x-dspro/x-soi-product"),
    productRequest ("x-dspro/x-soi-product-request"),
    topic ("x-dspro/x-soi-topic"),
    unitTask ("x-dspro/x-soi-unit-task"),
    trackInfo ("x-dspro/x-soi-track-info"),
    networkHealthMessage ("x-dspro/x-soi-network-health-message"),

    // Phoenix
    cot ("x-dspro/x-phoenix-cot"),
    trackInfoAir ("x-dspro/x-phoenix-track-info-air"),
    trackInfoGround ("x-dspro/x-phoenix-track-info-ground"),
    trackInfoSee ("x-dspro/x-phoenix-track-info-see");

    /**
     * Constructor
     * @param mimeType mime type value
     */
    DSProMimeType (String mimeType)
    {
        _mimeType = mimeType;
    }

    /**
     * Gets the mime type value
     * @return the mime type value
     */
    public String value()
    {
        return _mimeType;
    }

    /**
     * Gets the <code>DSProMimeType</code> corresponding exactly to the <code>String</code> passed as input, if any
     * @param mimeType mime type <code>String</code> to look for
     * @return the <code>DSProMimeType</code> corresponding to the <code>String</code> passed as input, if any
     */
    public static DSProMimeType getMatch (String mimeType)
    {
        if (mimeType == null) {
            return null;
        }

        for (DSProMimeType dmt : values()) {
            if (dmt._mimeType.equals (mimeType)) {
                return dmt;
            }
        }

        return null;
    }

    /**
     * Gets the <code>DSProMimeType</code> corresponding to the <code>String</code> passed as input ignoring the case,
     * if any
     * @param mimeType mime type <code>String</code> to look for
     * @return the <code>DSProMimeType</code> corresponding to the <code>String</code> passed as input, if any
     */
    public static DSProMimeType getMatchIgnoreCase (String mimeType)
    {
        if (mimeType == null) {
            return null;
        }

        for (DSProMimeType dmt : values()) {
            if (dmt._mimeType.toLowerCase().equals (mimeType.toLowerCase())) {
                return dmt;
            }
        }

        return null;
    }

    private final String _mimeType;
}
