package us.ihmc.gst.oa;

import java.util.List;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public abstract class ClassifiedMessage extends XMLMessage
{
    public enum SecurityLevel {
        U,  // Unclassified
        C,  // Confidential,
        S,  // Secret,
        T;  // Top Secret
    }

    private static final String ELEMENT = "Security";
    private static final String PROPERTY = "classification";
    private String _classification;

    public ClassifiedMessage()
    {
        super();
        _classification = SecurityLevel.C.toString();
    }

    public String getClassification()
    {
        return _classification;
    }

    @Override
    protected void readElement(List<String> elements, String val) {
        super.readElement (elements, val);
    }

    @Override
    protected void readProperty (List<String> elements, String property, String val)
    {
        if (elements == null || property == null || val == null ||
            elements.isEmpty() ||
            !ELEMENT.equalsIgnoreCase(elements.get(0)) ||
            !PROPERTY.equalsIgnoreCase(property)) {
            return;
        }
        try {
            _classification = SecurityLevel.valueOf (val).toString();
        } catch (IllegalArgumentException ex) {}
    }

}
