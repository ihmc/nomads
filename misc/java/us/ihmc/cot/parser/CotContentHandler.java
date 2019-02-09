package us.ihmc.cot.parser;

import org.apache.log4j.Logger;
import org.xml.sax.*;
import java.util.Date;
import java.util.Stack;

class CotContentHandler implements ContentHandler {

    private static final Logger Log = Logger.getLogger(CotContentHandler.class);

    private Stack<CotDetail> _detailStack = new Stack<CotDetail>();
    private boolean _finishedDetail;
    private StringBuilder _innerTextBuilder = new StringBuilder();
    private boolean _parsedPoint;
    private CotEvent editor;

    /**
     * Makes use of internal buffers and will not be able to be threaded.
     */
    public synchronized CotEvent parseXML (final String xml) throws SAXException
    {
        _detailStack.clear();
        _finishedDetail = false;
        _innerTextBuilder.setLength(0);
        _parsedPoint = false;

        editor = new CotEvent();
        Xml.parse (xml, this);

        /**
         * Because the Cot <detail> tag contains additional information about the CotPoint, we will
         * need to make one last pass through the CotEvent and fix up the internal representation of
         * the CotPoint. Attempts to make this part of the CotPoint, or CotDetails setting became to
         * messy. This logic is localized and package protected as to keep abuse down.
         */
        editor.fixCoordinates();
        return editor;

    }

    @Override
    public void characters(char[] ch, int start, int length) throws SAXException {

        // we get all the bs whitespace here too
        boolean isLegit = false;
        for (int i = start; i < start + length; ++i) {
            if (!Character.isWhitespace(ch[i])) {
                isLegit = true;
                break;
            }
        }

        // it is the value of a detail
        if (isLegit && _detailStack.size() > 0) {
            _innerTextBuilder.append(ch, start, length);
        }
    }

    @Override
    public void endDocument() throws SAXException {

    }

    @Override
    public void endElement(String uri, String localName, String qName) throws SAXException {
        if (_detailStack.size() > 0) {
            CotDetail detail = _detailStack.pop();
            if (_innerTextBuilder.length() > 0) {
                detail.setInnerText(_innerTextBuilder.toString());
                _innerTextBuilder.setLength(0);
            }
            if (_detailStack.size() == 0) {
                _finishedDetail = true;
            }
        }
    }

    @Override
    public void endPrefixMapping(String prefix) throws SAXException {

    }

    @Override
    public void ignorableWhitespace(char[] ch, int start, int length) throws SAXException {

    }

    @Override
    public void processingInstruction(String target, String data) throws SAXException {

    }

    @Override
    public void setDocumentLocator(Locator locator) {

    }

    @Override
    public void skippedEntity(String name) throws SAXException {

    }

    @Override
    public void startDocument() throws SAXException {

    }

    //TODO Checking why the ifs were considering localName (which was empty) instead of qname
    @Override
    public void startElement(String uri, String localName, String qName, Attributes attrs)
            throws SAXException {
        try {
            if (qName.equals("event") && _detailStack.size() == 0) {
                editor.setType(_stringOrThrow(attrs, "type", "event: missing type"));
                editor.setVersion(_stringOrFallback(attrs, "version", "2.0"));
                editor.setUid(_stringOrThrow(attrs, "uid", "event: missing uid"));
                editor.setTime(_timeOrDefault(attrs, "time", "event: illegal or missing time"));
                editor.setStart(_timeOrDefault(attrs, "start", "event: illegal or missing start"));
                editor.setStale(_timeOrDefault(attrs, "stale", "event: illegal or missing stale"));
                editor.setHow(_stringOrFallback(attrs, "how", ""));
                editor.setOpex(_stringOrFallback(attrs, "opex", null));
                editor.setQos(_stringOrFallback(attrs, "qos", null));
                editor.setAccess(_stringOrFallback(attrs, "access", null));
                // these might not be clear in the case that a recycled event was passed in
                editor.setPoint(CotPoint.ZERO);
                editor.setDetail(null);
            }
            else if (qName.equals("point") && _detailStack.size() == 0) {
                // if (_parsedPoint || _eventEditor == null) {
                // throw new CotIllegalException("illegal point tag");
                // }
                double lat = _doubleOrThrow(attrs, "lat", "point: illegal or missing lat");
                double lon = _doubleOrThrow(attrs, "lon", "point: illegal or missing lon");
                double hae = _doubleOrThrow(attrs, "hae", "point: illegal or missing hae");
                double le = _doubleOrFallback(attrs, "le", CotPoint.COT_LE90_UNKNOWN);
                double ce = _doubleOrFallback(attrs, "ce", CotPoint.COT_CE90_UNKNOWN);
                editor.setPoint(new CotPoint(lat, lon, hae, ce, le));
                _parsedPoint = true;
            }
            else if (qName.equals("detail") && _detailStack.size() == 0 && !_finishedDetail) {
                CotDetail detail = _pushDetail("detail", attrs);
                editor.setDetail(detail);
            }
            else if (_detailStack.size() > 0) {
                // inside of detail tag just get DOM'ed out
                _pushDetail(qName, attrs);
            }
        } catch (CotIllegalException e) {
            throw new SAXException(e.toString());
        }

    }

    @Override
    public void startPrefixMapping(String prefix, String uri) throws SAXException {

    }

    private CotDetail _pushDetail(String name, Attributes attrs) {
        CotDetail detail = new CotDetail();

        // set name and attributes
        detail.setElementName(name);
        //System.out.println ("Name: " + name + " attr.length: " + attrs.getLength());
        for (int i = 0; i < attrs.getLength(); ++i) {
            //String attrName = attrs.getLocalName(i);  // Rita: replaced getLocalName with getQName because getLocalName was always null
            String attrName = attrs.getQName (i);
            String attrValue = attrs.getValue(i);
            //System.out.println ("attrName: " + attrName + " attrValue: " + attrValue);
            detail.setAttribute(attrName, attrValue);
        }

        // add it to (any) parent
        if (_detailStack.size() > 0) {
            CotDetail parentDetail = _detailStack.peek();
            parentDetail.addChild(detail);
            //System.out.println ("Added child " + detail + " to parent " + parentDetail);
        }

        // push it on the stack
        _detailStack.push(detail);

        return detail;
    }

    private static CotTime _timeOrDefault(Attributes attrs, String name, String msg) {
        try {
            return CotTime.parseTime2(attrs.getValue(name));
        } catch (Exception ex) {
            Log.error("_timeOrDefault" + msg);
            return new CotTime(new Date());
        }
    }

    private static String _stringOrThrow(Attributes attrs, String name, String msg)
            throws CotIllegalException {
        String value = attrs.getValue(name);
        if (value == null) {
            throw new CotIllegalException(msg);
        }
        return value;
    }

    private static String _stringOrFallback(Attributes attrs, String name, String fallback) {
        String value = attrs.getValue(name);
        if (value == null) {
            value = fallback;
        }
        return value;
    }

    private static double _doubleOrThrow(Attributes attrs, String name, String msg)
            throws CotIllegalException {
        try {
            return Double.parseDouble(attrs.getValue(name));
        } catch (Exception ex) {
            throw new CotIllegalException(msg);
        }
    }

    private static double _doubleOrFallback(Attributes attrs, String name, double fallback) {
        try {
            return Double.parseDouble(attrs.getValue(name));
        } catch (Exception ex) {
            return fallback;
        }
    }
}
