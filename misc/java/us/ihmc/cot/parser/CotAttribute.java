package us.ihmc.cot.parser;

/**
 * An attribute of a detail
 * 
 * @author Joe Bergeron
 */
public class CotAttribute {

    private String _name;
    private String _value;

    /**
     * Create a new attribute
     * 
     * @param name the name of the attribute
     * @param value the value of the attribute
     */
    public CotAttribute(String name, String value) {
        _name = name;
        _value = value;
    }

    /**
     * Get this attribute's name
     * 
     * @return
     */
    public String getName() {
        return _name;
    }

    /**
     * Get this attribute's value
     * 
     * @return
     */
    public String getValue()
    {
        if (_value != null)
            return _value;
        else
            return "";
    }
}
