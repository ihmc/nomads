package us.ihmc.aci.util.dspro.soi;

/**
 * DataProductsExchangeFormat.java
 * <p/>
 * Class containing fields parsed from <code>InformationObject</code> instances of data product type.
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class DataProductsExchangeFormat extends ARLExchangeFormat
{
    public static final String DATA_PRODUCT_ID = "data.product.id";

    public static final String INF_DISPLAY_TEXT = "inf.display.text";
    public static final String INR_DISPLAY_TEXT = "inr.display.text";

    public static final String PRODUCT_TRACK_COUNT = "product.track.count";
    public static final String PRODUCT_TRACK = "product.track.number.";
    public static final String PRODUCT_TRACK_ENTITY_TYPE = ".entity.type";
    public static final String PRODUCT_TRACK_ID = ".id";
    public static final String PRODUCT_TRACK_LATITUDE = ".latitude";
    public static final String PRODUCT_TRACK_LONGITUDE = ".longitude";
    public static final String PRODUCT_TRACK_NAME = ".name";
    public static final String PRODUCT_TRACK_TIME = ".time";
    public static final String PRODUCT_TRACK_MIL_STD_2525_SYMBOL_ID = ".milStd2525SymbolId";
    public static final String PRODUCT_TRACK_TYPE = ".type";
    public static final String PRODUCT_TRACK_IMAGES_COUNT = ".images.count";
    public static final String PRODUCT_TRACK_IMAGE = ".image.number.";
    public static final String PRODUCT_TRACK_IMAGE_URL = ".url";
    public static final String PRODUCT_TRACK_IMAGE_URL_REFERENCE = ".url.reference.property.number.";
    public static final String PRODUCT_TRACK_VIDEO_COUNT = ".video.count";
    public static final String PRODUCT_TRACK_VIDEO = ".videos.number.";
    public static final String PRODUCT_TRACK_VIDEO_URL = ".url";
    public static final String PRODUCT_TRACK_VIDEO_URL_REFERENCE = ".url.reference.property.number.";
    public static final String PRODUCT_TRACK_VIDEO_REFERENCE_ENCODED_DATA_PROPERTY = ".reference.encoded.video.property";
//    public static final String PRODUCT_TRACK_VIDEO_REFERENCE_URL_PROPERTY = ".reference.url.property";  // Reference to the property IMAGE_URL_# containing the
//                                                                                                        // current video url.
//                                                                                                        // To be used as PRODUCT_TRACK + i + PRODUCT_TRACK_VIDEO + j +
//                                                                                                        // PRODUCT_TRACK_VIDEO_REFERENCE_URL_PROPERTY
//    public static final String PRODUCT_TRACK_VIDEO_REFERENCE_VIDEO_AS_BYTES_PROPERTY = ".reference.video.as.bytes.property"; // Reference to the property IMAGE_AS_BYTE_#
//                                                                                                                             // containing the current video url.
//                                                                                                                             // To be used as PRODUCT_TRACK + i + PRODUCT_TRACK_VIDEO + j +
//                                                                                                                             // PRODUCT_TRACK_VIDEO_REFERENCE_URL_PROPERTY

    public static final String PRODUCT_EDITOR_COUNT = "product.editor.count";
    public static final String PRODUCT_EDITOR = "product.editor.number.";
    public static final String PRODUCT_EDITOR_TYPE = ".type";
    public static final String PRODUCT_EDITOR_IMAGE_URL_COUNT = ".image.url.count";
    public static final String PRODUCT_EDITOR_IMAGE_URL = ".image.url.number.";
    public static final String PRODUCT_EDITOR_VIDEO_URL_COUNT = ".video.url.count";
    public static final String PRODUCT_EDITOR_VIDEO_URL = ".video.url.number.";
    public static final String PRODUCT_EDITOR_SEARCH_TERM_COUNT = ".search.term.count";
    public static final String PRODUCT_EDITOR_SEARCH_TERM = ".search.term.number.";
//    public static final String PRODUCT_EDITOR_VIDEO_REFERENCE_URL_PROPERTY = ".reference.url.property"; // Reference to the property IMAGE_URL_# containing the
//                                                                                                        // current video url.
//                                                                                                        // To be used as PRODUCT_EDITOR + i + PRODUCT_EDITOR_VIDEO_REFERENCE_URL_PROPERTY + j
//    public static final String PRODUCT_EDITOR_VIDEO_REFERENCE_VIDEO_AS_BYTES_PROPERTY = ".reference.video.as.bytes.property";   // Reference to the property IMAGE_AS_BYTE_#
//                                                                                                                                // containing the current video url.
//                                                                                                                                // To be used as PRODUCT_EDITOR + i +
//                                                                                                                                // PRODUCT_EDITOR_VIDEO_REFERENCE_VIDEO_AS_BYTES_PROPERTY + j

    public static final String PRODUCT_IMAGE_COUNT = "product.image.count";
    public static final String PRODUCT_IMAGE = "product.image.number.";
    public static final String PRODUCT_IMAGE_LATITUDE = ".latitude";
    public static final String PRODUCT_IMAGE_LONGITUDE = ".longitude";
    public static final String PRODUCT_IMAGE_MIME_TYPE = ".mimetype";
    public static final String PRODUCT_IMAGE_URL = ".url";
    public static final String PRODUCT_IMAGE_REFERENCE_ENCODED_DATA_PROPERTY = ".reference.encoded.image.property";
    public static final String PRODUCT_IMAGE_REFERENCE_URL_PROPERTY = ".reference.image.url.property";

    public static final String PRODUCT_VIDEO_COUNT = "product.video.count";
    public static final String PRODUCT_VIDEO = "product.video.number.";
    public static final String PRODUCT_VIDEO_LATITUDE = ".latitude";
    public static final String PRODUCT_VIDEO_LONGITUDE = ".longitude";
    public static final String PRODUCT_VIDEO_MIME_TYPE = ".mimetype";
    public static final String PRODUCT_VIDEO_URL = ".url";
    public static final String PRODUCT_VIDEO_REFERENCE_ENCODED_DATA_PROPERTY = ".reference.encoded.video.property";
    public static final String PRODUCT_VIDEO_REFERENCE_URL_PROPERTY = ".reference.video.url.property";
//    public static final String PRODUCT_VIDEO_REFERENCE_URL_PROPERTY = ".reference.url.property.";   // Reference to the property IMAGE_URL_# containing the
//                                                                                                    // current video url.
//                                                                                                    // To be used as PRODUCT_VIDEO + i + PRODUCT_VIDEO_REFERENCE_URL_PROPERTY
//    public static final String PRODUCT_VIDEO_REFERENCE_VIDEO_AS_BYTES_PROPERTY = ".reference.video.as.bytes.property";  // Reference to the property IMAGE_AS_BYTE_#
//                                                                                                                        // containing the current video url.
//                                                                                                                        // To be used as PRODUCT_VIDEO + i +
//                                                                                                                        // PRODUCT_VIDEO_REFERENCE_VIDEO_AS_BYTES_PROPERTY

    public static final String PRODUCT_REPORT_COUNT = "product.report.count";
    public static final String PRODUCT_REPORT = "product.report.number.";
    public static final String PRODUCT_REPORT_ID = ".id";
    public static final String PRODUCT_REPORT_LATITUDE = ".latitude";
    public static final String PRODUCT_REPORT_LONGITUDE = ".longitude";
    public static final String PRODUCT_REPORT_NAME = ".name";
    public static final String PRODUCT_REPORT_MIME_TYPE = ".mimetype";
    public static final String PRODUCT_REPORT_URL = ".url";
    public static final String PRODUCT_REPORT_REFERENCE_ENCODED_DATA_PROPERTY = ".reference.encoded.report.property";
//    public static final String PRODUCT_REPORT_REFERENCE_URL_PROPERTY = ".reference.url.property.";   // Reference to the property IMAGE_URL_# containing the
//                                                                                                    // current report url.
//                                                                                                    // To be used as PRODUCT_REPORT + i + PRODUCT_REPORT_REFERENCE_URL_PROPERTY
//    public static final String PRODUCT_REPORT_REFERENCE_REPORT_AS_BYTES_PROPERTY = ".reference.report.as.bytes.property";  // Reference to the property IMAGE_AS_BYTE_#
//                                                                                                                          // containing the current video url.
//                                                                                                                          // To be used as PRODUCT_REPORT + i +
//                                                                                                                          // PRODUCT_REPORT_REFERENCE_REPORT_AS_BYTES_PROPERTY

    public static final String DATA_URL = ImagesExchangeFormat.IMAGE_URL;
    public static final String DATA_AS_BYTES = ImagesExchangeFormat.IMAGE_AS_BYTES;

//    public static final String DSPRO_DESCRIPTION_HIGHEST_PRIORITY = "DataProducts-highest-priority";    // Moved to DSProMimeType
//    public static final String DSPRO_DESCRIPTION_MEDIUM_PRIORITY = "DataProducts-medium-priority";  // Moved to DSProMimeType
//    public static final String DSPRO_DESCRIPTION_LOWEST_PRIORITY = "DataProducts-lowest-priority";  // Moved to DSProMimeType
//    public static final String DSPRO_GHUB_MIME_TYPE = "x-dspro/x-soi-ghub-data-products";
//    public static final String DSPRO_DATA_PRODUCT_MIME_TYPE = "x-dspro/x-soi-data-products";    // Moved to DSProMimeType
}
