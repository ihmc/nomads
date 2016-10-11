package us.ihmc.gst.oa;

import java.util.Date;
import java.util.List;
import us.ihmc.gst.util.DateUtils;
import us.ihmc.gst.util.ElementComparator;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class UpdateMessage extends ClassifiedMessage
{
    public static final String MAGIC_STRING = "UpdateMessage";

    private Date _date;
    private String _actorId;

    private enum Element {
        DateTime,
        ActorID;
    }

    public UpdateMessage()
    {
        super();
        _actorId = "";
        _date = null;
    }

    @Override
    protected String getMagicString()
    {
        return MAGIC_STRING;
    }

    public Date getDate()
    {
        return _date;
    }

    public String getActorId()
    {
        return _actorId;
    }

    @Override
    protected void readElement(List<String> elements, String val) {
        super.readElement (elements, val);

        if (ElementComparator.matches (elements, Element.DateTime.name())) {
            _date = DateUtils.parseDate (val);
        }
        else if (ElementComparator.matches (elements, Element.ActorID.name())) {
            _actorId = val;
        }
    }

    @Override
    protected void readProperty(List<String> elements, String property, String val)
    {
        super.readProperty(elements, property, val);
    }

}
