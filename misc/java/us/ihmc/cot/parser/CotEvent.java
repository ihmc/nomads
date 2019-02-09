package us.ihmc.cot.parser;



import org.xml.sax.SAXException;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * Represents a Cursor on Target event
 * 
 * @see http://cot.mitre.org
 */
public class CotEvent {

    private final static int _OPTIONAL_DETAIL_BIT = 1;
    private final static int _OPTIONAL_OPEX_BIT = 1 << 1;
    private final static int _OPTIONAL_ACCESS_BIT = 1 << 2;
    private final static int _OPTIONAL_QOS_BIT = 1 << 3;

    final static CotContentHandler cotHandler = new CotContentHandler();

    public static final String TAG = "CotEvent";
    static PrintWriter fileWriter = null;

    // required
    private String _uid;
    private String _type;
    private String _vers;
    private CotPoint _point;
    private CotTime _time;
    private CotTime _start;
    private CotTime _stale;
    private String _how;

    // optional
    private CotDetail _detail;
    private String _opex;
    private String _qos;
    private String _access;

    /**
     * Default <i>version</i> attribute value (2.0)
     */
    public static final String VERSION_2_0 = "2.0";

    /**
     * A good default and value of a machine generated <i>how</i> attribute
     */
    public static final String HOW_MACHINE_GENERATED = "m-g";

    /**
     * Value of a human generated <i>how</i> attribute
     */
    public static final String HOW_HUMAN_GARBAGE_IN_GARBAGE_OUT = "h-g-i-g-o";

    /**
     * Create a default state event. The default state is missing the required attributes 'uid',
     * 'type', 'time', 'start', and 'stale'; The required attributes must be set before the event is
     * valid (isValid()). The required inner point tag is defaulted to 0, 0 (CotPoint.ZERO) and no
     * detail tag exists.
     */
    public CotEvent() {
        _vers = VERSION_2_0;
        _point = CotPoint.ZERO; // for backwards compatibility bug #1029
    }

    /**
     * Copy constructor.
     */
    public CotEvent(final String _uid, final String _type, final String _vers,
            final CotPoint _point, final CotTime _time,
            final CotTime _start, final CotTime _stale, final String _how,
            final CotDetail _detail,
            final String _opex,
            final String _qos, final String _access) {

        this._uid = _uid;
        this._type = _type;
        this._vers = _vers;
        this._point = _point;
        if (_detail != null) {
            this._detail = new CotDetail(_detail);
        }
        this._time = _time;
        this._start = _start;
        this._stale = _stale;
        this._how = _how;
        this._qos = _qos;
        this._opex = _opex;
        this._access = _access;
    }

    /**
     * Copy constructor. That is not crazy long
     */
    public CotEvent(CotEvent event) {

        this._uid = event._uid;
        this._type = event._type;
        this._vers = event._vers;
        if (event._point != null) {
            this._point = new CotPoint(event._point);
        }
        if (event._detail != null) {
            this._detail = new CotDetail(event._detail);
        }
        if (event._time != null) {
            this._time = new CotTime(event._time);
        }
        if (event._start != null) {
            this._start = new CotTime(event._start);
        }
        if (event._stale != null) {
            this._stale = new CotTime(event._stale);
        }

        this._how = event._how;
        this._qos = event._qos;
        this._opex = event._opex;
        this._access = event._access;
    }

    /**
     * Determine if the event is valid
     * 
     * @return
     */
    public boolean isValid() {
        return _uid != null &&
                !_uid.trim().equals("") &&
                _type != null &&
                !_type.trim().equals("") &&
                _time != null &&
                _start != null &&
                _stale != null &&
                _how != null &&
                !_how.trim().equals("") &&
                _point != null &&
                (_point.getLat() >= -90 && _point.getLat() <= 90) &&
                (_point.getLon() >= -180 && _point.getLon() <= 180);
    }

    /**
     * Retrieves a date as a formatted string useful for using as part of a file name.
     * 
     * @return a logging date/time string based on the current system time in yyyyMMdd_HHmm_ss
     *         format.
     */
    public static String getLogDateString() {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd_HHmm_ss",
                Locale.US);
        String dateString = sdf.format(new Date());
        return dateString;
    }

    /**
     * Location to log invalid CoT messages. For the purposes of ATAK this should be
     * FileSystemUtils.getItem("cot");
     */
    synchronized public static void setLogInvalid(boolean log, File directory) {
        // if logging is enabled, create the file to log to.

        if (log) {
            // trying to enable logging twice, use the existing
            // file to log to instead of creating a new one
            if (fileWriter != null)
                return;

            // File f = FileSystemUtils.getItem("cot");
            if (!directory.exists())
                directory.mkdir();
            try {
                File lf = new File(directory, "cot_" + getLogDateString()
                        + ".log");
                fileWriter = new PrintWriter(lf);
            } catch (FileNotFoundException fnfe) {
                fnfe.printStackTrace();
            }
        } else {
            if (fileWriter != null)
                fileWriter.close();
            fileWriter = null;
        }
    }

    /**
     * Get this 'version' attribute
     * 
     * @return a String indicating the version number
     */
    public String getVersion() {
        return _vers;
    }

    /**
     * Get this event 'type' attribute
     * 
     * @return
     */
    public String getType() {
        return _type;
    }

    /**
     * Get this event UID
     * 
     * @return
     */
    public String getUid() {
        return _uid;
    }

    /**
     * Get this event point as a CotPoint
     * 
     * @return
     */
    public CotPoint getCotPoint() {
        return _point;
    }

    /**
     * Get this event root detail
     * 
     * @return
     */
    public CotDetail getDetail() {
        return _detail;
    }

    /**
     * Get this event time
     * 
     * @return
     */
    public CotTime getTime() {
        return _time;
    }

    /**
     * Get this event start time
     * 
     * @return
     */
    public CotTime getStart() {
        return _start;
    }

    /**
     * Get this event stale time
     * 
     * @return
     */
    public CotTime getStale() {
        return _stale;
    }

    /**
     * Get this event how
     * 
     * @return
     */
    public String getHow() {
        return _how;
    }

    /**
     * @return
     */
    public String getOpex() {
        return _opex;
    }

    /**
     * @return
     */
    public String getQos() {
        return _qos;
    }

    /**
     * @return
     */
    public String getAccess() {
        return _access;
    }

    public void buildXml(StringBuffer b) {
        try {
            this.buildXmlImpl(b);
        } catch (IOException e) {
            throw new IllegalStateException(e);
        }
    }

    public void buildXml(StringBuilder b) {
        try {
            this.buildXmlImpl(b);
        } catch (IOException e) {
            throw new IllegalStateException(e);
        }
    }

    /**
     * @return
     */
    public void buildXml(Appendable b) throws IOException {
        this.buildXmlImpl(b);
    }

    private void buildXmlImpl(Appendable b) throws IOException {
        b.append("<?xml version='1.0' encoding='UTF-8' standalone='yes'?><event");
        if (_vers != null && !_vers.equals("")) {
            b.append(" version='");
            b.append(_vers);
            b.append("'");
        }
        b.append(" uid='");
        b.append(escapeXmlText(_uid));
        b.append("' type='");
        b.append(_type);
        b.append("' time='");
        b.append(_time.toString());
        b.append("' start='");
        b.append(_start.toString());
        b.append("' stale='");
        b.append(_stale.toString());
        b.append("' how='");
        b.append(_how);
        b.append("'");
        if (_opex != null) {
            b.append(" opex='");
            b.append(_opex);
            b.append("'");
        }
        if (_qos != null) {
            b.append(" qos='");
            b.append(_qos);
            b.append("'");
        }
        if (_access != null) {
            b.append(" access='");
            b.append(_access);
            b.append("'");
        }
        b.append(">");
        if (_point != null) {
            _point.buildXml(b);
        }
        if (_detail != null) {

            // dynamically insert the precise location information
            CotDetail preciselocation = null;
            if (_point != null) {
                preciselocation = new CotDetail("precisionlocation");
                preciselocation.setAttribute("altsrc",
                        _point.getAltitudeSource());
                preciselocation.setAttribute("geopointsrc",
                        _point.getGeoPointSource());

                _detail.addChild(preciselocation);
            }

            _detail.buildXml(b);

            // remove dynamically inserted additional location information.
            if (preciselocation != null)
                _detail.removeChild(preciselocation);
        }
        b.append("</event>");
    }

    /**
     * Parse a event from an XML string
     * 
     * @param xml
     * @return a CoT Event that can either be valid or invalid.
     */
    public static CotEvent parse(String xml) throws SAXException {
        CotEvent e = cotHandler.parseXML(xml);

        if (!e.isValid()) {
            System.out.println("INVALID CoTEVENT");
        }

        /**
         * If the CotEvent is not valid, we should probably record it to a file if CotLogging is
         * enabled.
         */

        if (fileWriter != null) {
            if (!e.isValid()) {
                try {
                    fileWriter.println(xml);
                } catch (Exception ex) {
                    // instead of synchronizing this to death, just catch the
                    // potential npe.
                }
            }
        }

        return e;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        buildXml(sb);
        return sb.toString();
    }

    static String escapeXmlText(String innerText) {

        if (innerText == null) {
            innerText = "";
        }

        StringBuilder sb = new StringBuilder(innerText);
        for (int i = 0; i < sb.length();) {
            char ch = sb.charAt(i);
            switch (ch) {
                case '&':
                    sb.replace(i, i + 1, "&amp;");
                    i += 5;
                    break;
                case '<':
                    sb.replace(i, i + 1, "&lt;");
                    i += 4;
                    break;
                case '>':
                    sb.replace(i, i + 1, "&gt;");
                    i += 4;
                    break;
                case '"':
                    sb.replace(i, i + 1, "&quot;");
                    i += 6;
                    break;
                case '\'':
                    sb.replace(i, i + 1, "&apos;");
                    i += 6;
                    break;
                case '\n':
                    sb.replace(i, i + 1, "&#10;");
                    i += 5;
                    break;
                default:
                    ++i;
                    break;
            }
        }
        return sb.toString();
    }

    /**
     * Set the version attribute. If not set, the value by default is "2.0".
     * 
     * @param vers
     */
    public void setVersion(String vers) {
        if (vers == null || vers.trim().equals("")) {
            throw new IllegalArgumentException("version may not be nothing");
        }
        _vers = vers;
    }

    /**
     * Set the CoT type (e.g. a-f-G).
     * 
     * @param type
     */
    public void setType(String type) {
        if (type == null || type.trim().equals("")) {
            throw new IllegalArgumentException("type may not be nothing");
        }
        _type = type;
    }

    /**
     * Set the unique identifier for the object the event describes
     * 
     * @param uid
     */
    public void setUid(String uid) {
        if (uid == null || uid.trim().equals("")) {
            throw new IllegalArgumentException("uid may not be nothing");
        }
        _uid = uid;
    }

    /**
     * Set the point tag details
     * 
     * @throws IllegalArgumentException if point is null
     * @param point
     */
    public void setPoint(CotPoint point) {
        if (point == null) {
            throw new IllegalArgumentException(
                    "point attribute may not be null");
        }
        _point = point;
    }

    /**
     * Set the detail tag. This must be named "detail" or be null.
     * 
     * @throws IllegalArgumentException if the CotDetail element name isn't "detail"
     * @param detail
     */
    public void setDetail(CotDetail detail) {
        if (detail != null && detail.getElementName() != null
                && !detail.getElementName().equals("detail")) {
            throw new IllegalArgumentException(
                    "detail tag must be named 'detail' (got '"
                            + detail.getElementName() + "'");
        }
        _detail = detail;

    }

    /**
     * Package protected fix up code for adding in the altitude source after the details have been
     * read from a cot event. Only used by CotContentParser. THIS METHOD SHOULD NEVER BE PUBLIC
     */
    void fixCoordinates() {
        if (_detail != null) {
            CotDetail ploc = _detail
                    .getFirstChildByName(0, "precisionlocation");
            if (ploc != null) {
                _point.setAltitudeSource(ploc.getAttribute("altsrc"));
                _point.setGeoPointSource(ploc.getAttribute("geopointsrc"));
            }
            _detail.removeChild(ploc);
        }
    }

    /**
     * Set the time this event was generated
     * 
     * @param time
     */
    public void setTime(CotTime time) {

        if (time == null) {
            throw new IllegalArgumentException(
                    "time attribute must not be null");
        }
        _time = time;
    }

    /**
     * Set the time this event starts scope
     * 
     * @param start
     */
    public void setStart(CotTime start) {
        if (start == null) {
            throw new IllegalArgumentException(
                    "start attribute must not be null");
        }
        _start = start;
    }

    /**
     * Set the time this event leaves from scope
     * 
     * @param stale
     */
    public void setStale(CotTime stale) {
        if (stale == null) {
            throw new IllegalArgumentException(
                    "stale attribute must not be null");
        }
        _stale = stale;
    }

    /**
     * Set the 'how' attribute of the event (e.g. m-g)
     * 
     * @param how
     */
    public void setHow(String how) {
        if (how == null || how.trim().equals("")) {
            throw new IllegalArgumentException(
                    "how attribute must not be nothing");
        }
        _how = how;
    }

    /**
     * Set the 'opex' attribute of the event
     * 
     * @param opex
     */
    public void setOpex(String opex) {
        _opex = opex;
    }

    /**
     * Set the 'qos' (quality of service) attribute of the event
     * 
     * @param qos
     */
    public void setQos(String qos) {
        _qos = qos;
    }

    /**
     * Set the 'access' attribute of the event
     * 
     * @param access
     */
    public void setAccess(String access) {
        _access = access;
    }

}
