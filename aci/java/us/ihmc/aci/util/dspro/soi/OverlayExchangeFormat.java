package us.ihmc.aci.util.dspro.soi;

/**
 * OverlayExchangeFormat.java
 *
 * Class containing fields parsed from <code>InformationObject</code> instances of graphic (overlay) type.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class OverlayExchangeFormat extends ExchangeFormat
{
    public static final String RESOURCE_OVERLAYS_NUMBER = "resource.overlaysNumber";

    public static final String OVERLAY_TYPE = "overlay.type"; //CUSTOM PROPERTY
    public static final String OVERLAY_MIL_ID = "overlay.milId";
    public static final String OVERLAY_NAME = "overlay.name";
    public static final String OVERLAY_LINE_COLOR = "overlay.lineColor";
    public static final String OVERLAY_FILL_COLOR = "overlay.fillColor";
    public static final String OVERLAY_TEXT = "overlay.text";
    public static final String OVERLAY_CUSTOM_TOOLTIP = "overlay.customTooltip";
    public static final String OVERLAY_TIME_ALERT_DGT = "overlay.timeAlertDgt";
    public static final String OVERLAY_MODIFIED_TIME = "overlay.modifiedTime";
    public static final String OVERLAY_CREATE_TIME = "overlay.createTime";
    public static final String OVERLAY_POSITION_N = "overlay.position."; //add number to identify position
    public static final String OVERLAY_POINTS_SAVED = "overlay.pointsSaved";
    public static final String OVERLAY_RADIUS = "overlay.radius";
    public static final String OVERLAY_CENTER = "overlay.center";
    public static final String OVERLAY_LABEL_POSITION = "overlay.labelPosition";

//    public static final String DSPRO_OVERLAY_DESCRIPTION = "Overlay";    // Moved to DSProMimeType
//    public static final String DSPRO_OVERLAY_MIME_TYPE = "x-dspro/x-soi-overlay";   // Moved to DSProMimeType

    public static final int LENGTH_CHECK_POINT = 1;
    public static final int LENGTH_PHASE_LINE = 2;
    public static final int MIN_LENGTH_ASSEMBLY_AREA = 3; //min supported positions
}
