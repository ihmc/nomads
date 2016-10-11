package us.ihmc.aci.util.dspro.soi;

/**
 * IntelDataExchangeFormat.java
 * <p/>
 * Class containing fields parsed from <code>InformationObject</code> instances of GHub data type.
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class IntelDataExchangeFormat extends ImagesExchangeFormat
{
    public static final String NAME = "ghub.data.name";
    public static final String ID = "id";
    //public static final String URLS_COUNT = "urls.count";
    //public static final String URL = "url.number.";
    //public static final String IMAGE_AS_BYTES = "image.as.bytes.number.";
    public static final String TIME = "time";
    public static final String CREATION_TIME = "creation.time";
    public static final String LOCATION_LATITUDE = "latitude";
    public static final String LOCATION_LONGITUDE = "longitude";
    public static final String REFERENCES_COUNT = "references.count";
    public static final String REFERENCE = "reference.number.";

//    public static final String DSPRO_GHUB_DESCRIPTION = "GHUB-IntelData";   // Moved to DSProMimeType
//    public static final String DSPRO_GHUB_MIME_TYPE = "x-dspro/x-soi-ghub-intel-data";  // Moved to DSProMimeType
}
