package us.ihmc.aci.envMonitor.provider.arl;

import java.io.InputStream;

import org.apache.xerces.parsers.DOMParser;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.xml.sax.InputSource;

/**
 * XMLMessageParser
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Aug 1, 2004 at 3:28:01 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class XMLMessageParser
{
    /**
     * Creates a new Instance of the XMLMessageParser
     * @throws Exception
     */
    public XMLMessageParser()
        throws Exception
    {
        _domParser = new DOMParser();
    }

    public void parseGPSInfo (GPSInfo gpsInfo, InputStream  inputStream)
    {
        try {
            if (_resetOnEachUpdate) {
                gpsInfo.reset();
            }
            _domParser.parse(new InputSource(inputStream));
            _xmlDocument = _domParser.getDocument();
            parseGPSInfo (gpsInfo, _xmlDocument);
            debugMsg(gpsInfo.toString());
        }  catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Walks the xml response issued by the ARL's POS server.
     * @param gpsInfo
     * @param node
     */
    public void parseGPSInfo(GPSInfo gpsInfo, Node node)
    {
        short type = node.getNodeType();
        if (type == Node.DOCUMENT_NODE) {
            node = node.getFirstChild();
            if (node.getNodeName().equals("DeviceData")) {
                Node child = node.getFirstChild();
                while (child != null) {
                    debugMsg("---->" + child.getNodeName());
                    if (child.getNodeType() == Node.ELEMENT_NODE) {
                        Node child2 = child.getFirstChild();
                        while (child2 != null) {
                            if (child2.getNodeType() == Node.ELEMENT_NODE) {
                                String label = child2.getNodeName();
                                debugMsg("\t---->" + child2.getNodeName());
                                Node child3 = child2.getFirstChild();
                                if (child3.getNodeType() == Node.TEXT_NODE) {
                                    String value = child3.getNodeValue();
                                    debugMsg("\t\t---->" + value);
                                    update (gpsInfo, label, value);
                                }
                            }
                            child2 = child2.getNextSibling();
                        }
                    }
                    child = child.getNextSibling();
                }
            }
        }
    }
    private void update (GPSInfo gpsInfo, String label, String value)
    {
        try {
            debugMsg("Inside_Update: (" + label + ", " + value + ")\n");
            if (label.equals("Pitch")) {
                debugMsg("Pitch -> " + value);
                value = verifyValue (value);
                gpsInfo.setPitch(Double.parseDouble(value));
            }
            else if (label.equals("Yaw")) {
                debugMsg("Yaw -> " + value);
                value = verifyValue (value);
                gpsInfo.setYaw(Double.parseDouble(value));
            }
            else if (label.equals("Roll")) {
                debugMsg("Roll -> " + value);
                value = verifyValue (value);
                gpsInfo.setRoll(Double.parseDouble(value));
            }
            else if (label.equals("Latitude")) {
                debugMsg("Latitude -> " + value);
                value = verifyValue (value);
                double dlat = Double.parseDouble(value);
                if (dlat == 0) {
                    dlat = 999.0;
                }
                gpsInfo.setLatitude(dlat);
            }
            else if (label.equals("Longitude")) {
                debugMsg("Longitude -> " + value);
                value = verifyValue (value);
                double dlong = Double.parseDouble(value);
                if (dlong == 0) {
                    dlong = 999.0;
                }
                gpsInfo.setLongitude(dlong);
            }
            else if (label.equals("Altitude")) {
                debugMsg("Altitude -> " + value);
                value = verifyValue (value);
                gpsInfo.setAltitude(Double.parseDouble(value));
            }
        }
        catch (Exception e) {
            debugMsg ("update method: Got Exception " + e.getMessage());
            e.printStackTrace();
        }
    }

    private String verifyValue (String value)
    {
        debugMsg ("Got (" + value + ") for verification");
        if (value != null) {
            int ipos = value.indexOf("-");
            if (ipos >0) {
                debugMsg ("\treturning (" + value + ") from verification");
                return (value.substring(ipos));
            }
        }
        debugMsg ("\treturning (" + value + ") from verification");
        return value;
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[XMLParser] " + msg);
        }
    }

    private boolean _debug = false;
    private boolean _resetOnEachUpdate = false;
    private DOMParser _domParser;
    private Document _xmlDocument;
}
