package us.ihmc.aci.util.dspro.soi;

/**
 * Collection of description values used in the interface with DSPro
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/25/2016
 */
public enum DSProDescription
{
    track ("Track"),
    overlay ("Overlay"),
    sigact ("SIGACT"),
    alertHighPriority ("MissionAlert-highest-priority"),
    alertMediumPriority ("MissionAlert-medium-priority"),
    alertLowPriority ("MissionAlert-lowest-priority"),
    dataProductHighPriority ("DataProducts-highest-priority"),
    dataProductMediumPriority ("DataProducts-medium-priority"),
    dataProductLowPriority ("DataProducts-lowest-priority"),
    logstat ("LogStat"),
    infoFulfillment ("InformationFulfillment"),
    infoRequirement ("InformationRequirement"),
    fuelReport ("FuelReport"),
    rapidRequest ("RapidRequest"),
    telemetryRequest ("TelemetryRequest"),
    telemetryUpdate ("TelemetryUpdate"),
    intel ("GHUB-IntelData"),
    image ("Image"),
    raw ("raw-soi-data"),
    arbitrary ("Arbitrary SOI message"),

    // SOI Gen 2
    dataProduct ("DataProduct"),
    enrichedInformationRequirement ("EnrichedInformationRequirement"),
    informationDeficit ("InformationDeficit"),
    informationFulfillmentRegistry ("InformationFulfillmentRegistry"),
    informationUnfilled ("InformationUnfilled"),
    intelEvent ("IntelEvent"),
    intelReport ("IntelReport"),
    mission ("Mission"),
    missionAlert ("MissionAlert"),
    product ("Product"),
    productRequest ("ProductRequest"),
    topic ("Topic"),
    unitTask ("UnitTask"),
    trackInfo ("TrackInfo"),
    networkHealthMessage ("NetworkHealthMessage"),

    // Phoenix
    cot ("CoT");

    /**
     * Constructor
     * @param description dspro description
     */
    DSProDescription (String description)
    {
        _description = description;
    }

    /**
     * Gets the description value
     * @return the description value
     */
    public String value()
    {
        return _description;
    }

    /**
     * Gets the <code>DSProDescription</code> corresponding exactly to the <code>String</code> passed as input, if any
     * @param description mime type <code>String</code> to look for
     * @return the <code>DSProDescription</code> corresponding to the <code>String</code> passed as input, if any
     */
    public static DSProDescription getMatch (String description)
    {
        if (description == null) {
            return null;
        }

        for (DSProDescription dd : values()) {
            if (dd._description.equals (description)) {
                return dd;
            }
        }

        return null;
    }

    /**
     * Gets the <code>DSProDescription</code> corresponding to the <code>String</code> passed as input ignoring the case,
     * if any
     * @param description mime type <code>String</code> to look for
     * @return the <code>DSProDescription</code> corresponding to the <code>String</code> passed as input, if any
     */
    public static DSProDescription getMatchIgnoreCase (String description)
    {
        if (description == null) {
            return null;
        }

        for (DSProDescription dd : values()) {
            if (dd._description.toLowerCase().equals (description.toLowerCase())) {
                return dd;
            }
        }

        return null;
    }

    private final String _description;
}
