package us.ihmc.gst.oa;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamReader;
import mil.navy.nrlssc.gst.oa.OAMessage;
import us.ihmc.gst.util.GeoCoordinates;

/**
 *
 * @author Giacomo Benincasa        (gbenincasa@ihmc.us)
 */
public abstract class XMLMessage extends OAMessage
{
    protected double _upperLeftLatitude;
    protected double _upperLeftLongitude;
    protected double _lowerRightLatitute;
    protected double _lowerRightLongitude;

    private String _refersToObj;

    private enum Element {
        RelatedLink
    } 

    protected XMLMessage()
    {
        super();
        _upperLeftLatitude = GeoCoordinates.UNSET_LAT;
        _upperLeftLongitude = GeoCoordinates.UNSET_LONG;
        _lowerRightLatitute = GeoCoordinates.UNSET_LAT;
        _lowerRightLongitude = GeoCoordinates.UNSET_LONG;
    }

    @Override
    public boolean deserialize (byte[] messageBuffer) {
        try {
            XMLInputFactory f = XMLInputFactory.newInstance();
            ByteArrayInputStream is = new ByteArrayInputStream (messageBuffer);
            XMLStreamReader r = f.createXMLStreamReader (is);
            boolean rc = parse (r);
            // The message has already been parsed, I do not care about exceptions!
            try { is.close(); } catch (IOException ex) {}
            try { r.close(); } catch (XMLStreamException ex) {}
            return rc;
        }
        catch (XMLStreamException ex) {
            Logger.getLogger (XMLMessage.class.getName()).log(Level.WARNING, null, ex);
            return false;
        }
    }

    public String getReferredObjectURL()
    {
        return _refersToObj;
    }

    @Override
    public byte[] serialize() throws IOException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void printMessage()
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    private boolean parse (XMLStreamReader r)
    {
        try {
            LinkedList<String> elements = new LinkedList<String>();
            String val = null;
            while (r.hasNext()) {
                switch (r.next()) {
                    case XMLStreamReader.END_DOCUMENT:
                        break;

                    case XMLStreamReader.START_DOCUMENT:
                        if (!getMagicString().equalsIgnoreCase(r.getLocalName())) {
                            return false;
                        }
                        break;

                    case XMLStreamReader.START_ELEMENT:
                        elements.push (r.getLocalName());
                        parseProperties (elements, r);
                        break;

                    case XMLStreamReader.END_ELEMENT:
                        elements.pop();
                        break;

                    case XMLStreamReader.CHARACTERS:
                        val = r.getText().trim();
                        readElement (elements, val);
                        val = null;
                        break;
                }
            }
            return true;
        }
        catch (XMLStreamException ex) {
            Logger.getLogger(XMLMessage.class.getName()).log(Level.SEVERE, null, ex);
            return false;
        }
    }

    private void parseProperties (List<String> elements, XMLStreamReader r)
    {
        for (int i = 0; i < r.getAttributeCount(); i++) {
            readProperty (elements,
                          r.getAttributeLocalName (i),
                          r.getAttributeValue (i).trim());
        }
    }

    protected void readElement (List<String> elements, String val)
    {
        String el = elements.get(0);
        if (el != null &&
            Element.RelatedLink.toString().equalsIgnoreCase (el)) {
            _refersToObj = val;
        }
    }

    protected abstract void readProperty (List<String> elements, String property, String val);

    protected abstract String getMagicString();
}

