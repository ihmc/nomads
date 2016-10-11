package us.ihmc.gst.oa;

import java.util.Date;
import java.util.List;
import us.ihmc.gst.util.DateUtils;
import us.ihmc.gst.util.ElementComparator;
import us.ihmc.gst.util.GeoCoordinates;

/**
 * <?xml version="1.0" encoding="UTF-8"?>
 * <ltsn:SensorStatusMessage
 *        xmlns:ism="urn:us:gov:ic:ism:v2" 
 *        xmlns:ltsn="http://ltsn.onr.navy.mil/messaging/" 
 *        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *        xsi:schemaLocation="http://ltsn.onr.navy.mil/messaging/ MessageTypes.xsd ">
 *  <ltsn:Security ism:classification="U" ism:ownerProducer="USA"/>
 *  <MessageID>100000000</MessageID>
 *  <CommDeviceID>PR_EO1</CommDeviceID>
 *  <UnitID>0</UnitID>
 *  <Time>2001-12-31T12:00:00</Time>
 *  <Location>
 *    <Latitude>30.0</Latitude>
 *    <Longitude>-90.0</Longitude>
 *    <AltitudeInMeters>1000.0</AltitudeInMeters>
 *    <AltitudeUncertaintyInMeters>5.0</AltitudeUncertaintyInMeters>
 *    <LocationUncertainty>
 *      <SemiAxis1InMeters>100.0</SemiAxis1InMeters>
 *      <SemiAxis2InMeters>50.0</SemiAxis2InMeters>
 *      <Axis1Orientation>270.0</Axis1Orientation>
 *    </LocationUncertainty>
 *  </Location>
 *  <Mission>Vignette01</Mission>
 *  <Capability>EO Biometric ID and Tracking</Capability>
 *  <FieldOfView>
 *        <Quadrilateral>
 *            <Center>
 *                <Latitude>35.714628</Latitude>
 *                <Longitude>-120.763259</Longitude>
 *            </Center>
 *            <Point>
 *                <Latitude>35.714867</Latitude>
 *                <Longitude>-120.763430</Longitude>
 *            </Point>
 *            <Point>
 *                <Latitude>35.714906</Latitude>
 *                <Longitude>-120.763235</Longitude>
 *            </Point>
 *            <Point>
 *                <Latitude>35.714499</Latitude>
 *                <Longitude>-120.762917</Longitude>
 *            </Point>
 *            <Point>
 *                <Latitude>35.714351</Latitude>
 *                <Longitude>-120.763383</Longitude>
 *            </Point>
 *        </Quadrilateral>
 *  </FieldOfView>
 *  <FieldOfRegard>
 *    <CenterPointWithRadius>
 *      <Center>
 *        <Latitude>35.714628</Latitude>
 *        <Longitude>-120.763259</Longitude>
 *      </Center>
 *      <RadiusInMeters>30.0</RadiusInMeters>
 *    </CenterPointWithRadius>
 *  </FieldOfRegard>
 * </ltsn:SensorStatusMessage>
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class SensorStatusMessage extends ClassifiedMessage
{
    public static final String MAGIC_STRING = "SensorStatusMessage";

    private Location _location;
    private Date _date;
    private String _messageId;
    private String _unitId;
    private String _mission;

    private enum Element {
        Location,
        Latitude,
        Longitude,        // Shall I parse filed of view instead???
        AltitudeInMeters,
        Time,
        UnitID,
        MessageID,
        Mission;
    }

    public SensorStatusMessage()
    {
        super();
        _location = new Location();
        _date = null;
        _messageId = "";
        _unitId = "";
        _mission = "";
    }

    @Override
    protected String getMagicString()
    {
        return MAGIC_STRING;
    }

    public String getMessageId()
    {
        return _messageId;
    }

    public String getMission()
    {
        return _mission;
    }

    public Location getLocation()
    {
        return _location;
    }

    public Date getDate()
    {
        return _date;
    }

    public String getUnitId()
    {
        return _unitId;
    }

    @Override
    protected void readElement(List<String> elements, String val) {
        super.readElement (elements, val);

        if (ElementComparator.matches (elements, Element.Latitude.name(), Element.Location.name())) {
            // Latitude is in the ] -90.0, 90.0[ range
            Double coord = Double.parseDouble (val);
            if (GeoCoordinates.isValidLatitude (coord)) {
                _location._latitude = coord.floatValue();
            }
        }
        else if (ElementComparator.matches (elements, Element.Longitude.name(), Element.Location.name())) {
            // Longitude is in the ] -180.0, 180.0[ range
            Double coord = Double.parseDouble (val);
            if (GeoCoordinates.isValidLongitude (coord)) {
                _location._longitude = coord.floatValue();
            }
        }
        else if (ElementComparator.matches (elements, Element.AltitudeInMeters.name(), Element.Location.name())) {
            
        }
        else if (ElementComparator.matches (elements, Element.Time.name())) {
            _date = DateUtils.parseDate (val);
        }
        else if (ElementComparator.matches (elements, Element.UnitID.name())) {
            _unitId = val;
        }
        else if (ElementComparator.matches (elements, Element.MessageID.name())) {
            _messageId = val;
        }
        else if (ElementComparator.matches (elements, Element.Mission.name())) {
            _mission = val;
        }
    }

    @Override
    protected void readProperty(List<String> elements, String property, String val)
    {
        super.readProperty(elements, property, val);
    }

}
