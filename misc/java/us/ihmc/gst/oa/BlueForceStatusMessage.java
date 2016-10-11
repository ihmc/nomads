package us.ihmc.gst.oa;

import java.util.Date;
import java.util.List;
import us.ihmc.gst.util.DateUtils;
import us.ihmc.gst.util.ElementComparator;
import us.ihmc.gst.util.GeoCoordinates;

/**
 * <?xml version="1.0" encoding="UTF-8"?>
 * <ltsn:BlueForceStatusMessage xmlns:ism="urn:us:gov:ic:ism:v2"
 *                              xmlns:ltsn="http://ltsn.onr.navy.mil/messaging/"
 *                              xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *                              xsi:schemaLocation="http://ltsn.onr.navy.mil/messaging/ GreenDevil/MessageTypes.xsd ">
 *   <ltsn:Security ism:ownerProducer="USA" ism:releasableTo="USA" ism:classification="U" />
 *   <MessageID>0</MessageID>
 *   <CommDeviceID>CommDeviceID</CommDeviceID>
 *   <UnitID>0</UnitID>
 *   <Time>2001-12-31T12:00:00</Time>
 *   <Location>
 *     <Latitude>0.0</Latitude>
 *     <LatitudeUncertainty>0.0</LatitudeUncertainty>
 *     <Longitude>0.0</Longitude>
 *     <LongitudeUncertainty>0.0</LongitudeUncertainty>
 *     <Altitude>0.0</Altitude>
 *     <AltitudeUncertainty>0.0</AltitudeUncertainty>
 *     <AltitudeUnits>Meters</AltitudeUnits>
 *   </Location>
 *   <Mission>Mission</Mission>
 *   <Capability>Capability</Capability>
 * </ltsn:BlueForceStatusMessage>
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class BlueForceStatusMessage extends ClassifiedMessage
{
    public static final String MAGIC_STRING = "BlueForceStatusMessage";

    private Date _date;
    private Location _location;
    private String _messageId;
    private String _unitId;
    private String _mission;

    private enum Element {
        Location,
        Latitude,
        Longitude,
        Altitude,
        AltitudeUnits,
        Time,
        UnitID,
        MessageID,
        Mission;
    }

    public BlueForceStatusMessage()
    {
        super();
        _date = null;
        _location = new Location();
        _messageId = "";
        _unitId = "";
        _mission = "";
    }

    public Date getDate()
    {
        return _date;
    }

    public String getUnitId()
    {
        return _unitId;
    }

    public Location getLocation()
    {
        return _location;
    }

    public String getMessageId()
    {
        return _messageId;
    }

    public String getMission()
    {
        return _mission;
    }

    @Override
    protected String getMagicString()
    {
        return MAGIC_STRING;
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
        else if (ElementComparator.matches (elements, Element.Altitude.name(), Element.Location.name())) {
            
        }
        else if (ElementComparator.matches (elements, Element.Time.name())) {
            _date = DateUtils.parseDate(val);
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
