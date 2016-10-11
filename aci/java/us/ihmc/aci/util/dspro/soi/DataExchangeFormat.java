package us.ihmc.aci.util.dspro.soi;

/**
 * DataExchangeFormat.java
 * <p/>
 * Class containing fields parsed from <code>InformationObject</code> instances of <code>Data</code> type.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DataExchangeFormat extends ExchangeFormat
{
    public static final String DATA_IMAGE = "data.image";
    public static final String DATA_ARBITRARY_SOI_MESSAGE_CONTENT = "data.arbitrary.soi.message.content";

    public static final String DATA_METADATA = "data.metadata";
    public static final String DATA_NAME = "data.name";
    public static final String DATA_LATITUDE = "data.latitude";
    public static final String DATA_LONGITUDE = "data.longitude";

//    public static final String DSPRO_IMAGE_DESCRIPTION = "Image";   // Moved to DSProMimeType
//    public static final String DSPRO_IMAGE_JPEG_MIME_TYPE = "image/jpeg";   // Moved to DSProMimeType
//    public static final String DSPRO_IMAGE_PNG_MIME_TYPE = "image/png"; // Moved to DSProMimeType
//    public static final String DSPRO_IMAGE_BMP_MIME_TYPE = "image/bmp"; // Moved to DSProMimeType
//    public static final String DSPRO_IMAGE_GIF_MIME_TYPE = "image/gif"; // Moved to DSProMimeType

//    public static final String DSPRO_ARBITRARY_SOI_MESSAGE_CONTENT_DESCRIPTION = "Arbitrary SOI message";   // Moved to DSProMimeType
//    public static final String DSPRO_ARBITRARY_SOI_MESSAGE_CONTENT_MIME_TYPE = "x-dspro/arbitrary-soi-message"; // Moved to DSProMimeType
}
