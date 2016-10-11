package us.ihmc.gst.oa;

import java.util.Date;
import java.util.List;
import us.ihmc.gst.util.DateUtils;
import us.ihmc.gst.util.ElementComparator;

/**
 * <?xml version="1.0" encoding="UTF-8"?>
 * <ltsn:BlueForceSaluteMessage xmlns:ltsn="http://ltsn.onr.navy.mil/messaging/"
 *      xmlns:ism="urn:us:gov:ic:ism:v2" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *      xsi:schemaLocation="http://ltsn.onr.navy.mil/messaging/ GreenDevil/MessageTypes.xsd ">
 *     <Security ism:ownerProducer="USA" ism:releasableTo="USA" ism:classification="U" />
 *     <MessageID>0</MessageID>
 *     <CommDeviceID>CommDeviceID</CommDeviceID>
 *     <UnitID>0</UnitID>
 *     <Size>Size</Size>
 *     <Activity>Activity</Activity>
 *     <Location>Lat:35.721655,Lon:-120.775488</Location>
 *     <Unit>Unit</Unit>
 *     <Time>2001-12-31T12:00:00</Time>
 *     <Equipment>Equipment</Equipment>
 * </ltsn:BlueForceSaluteMessage>
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class BlueForceSaluteMessage extends ClassifiedMessage
{
    public static final String MAGIC_STRING = "BlueForceSaluteMessage";

    private Location _location;
    private String _unitID;
    private String _messageID;
    private Date _date;

    private enum Element {
        Location,
        Time,
        UnitID,
        MessageID;
    }

    public BlueForceSaluteMessage()
    {
        super();
        _location = new Location();
        _unitID= "";
        _messageID = "";
        _date = null;
    }

    @Override
    protected String getMagicString()
    {
        return MAGIC_STRING;
    }

    public String getMessageId()
    {
        return _messageID;
    }

    public String getUnitId()
    {
        return _unitID;
    }

    public Location getLocation()
    {
        return _location;
    }

    public Date getDate()
    {
        return _date;
    }

    @Override
    protected void readElement(List<String> elements, String val) {
        super.readElement (elements, val);

        if (ElementComparator.matches (elements, Element.Location.name())) {
            readLocation (val);
        }
        else if (ElementComparator.matches (elements, Element.Time.name())) {
            _date = DateUtils.parseDate (val);
        }
        else if (ElementComparator.matches (elements, Element.UnitID.name())) {
            _unitID = val;
        }
        else if (ElementComparator.matches (elements, Element.MessageID.name())) {
            _messageID = val;
        }
    }

    private void readLocation (String location)
    {
        //Lat:35.721655,Lon:-120.775488
        if (!location.contains(",")) {
           return; 
        }

        String[] coords = location.split(",");

        coords[0] = coords[0].trim();
        String[] tokens = coords[0].split(":");
        if (tokens[0].trim().equalsIgnoreCase ("Lat")) {
            _location._latitude = (new Double(tokens[1])).floatValue();
        }
        else {
            _location._longitude = (new Double(tokens[1])).floatValue();
        }

        coords[1] = coords[1].trim();
        tokens = coords[1].split(":");
        if (tokens[0].trim().equalsIgnoreCase ("Lat")) {
            _location._latitude = (new Double(tokens[1])).floatValue();
        }
        else {
            _location._longitude = (new Double(tokens[1])).floatValue();
        }
    }

    @Override
    protected void readProperty(List<String> elements, String property, String val)
    {
        super.readProperty(elements, property, val);
    }
}
