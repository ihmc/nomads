package us.ihmc.android.aci.dspro.util;

import java.util.Collections;
import java.util.Enumeration;
import java.util.LinkedHashSet;
import java.util.Properties;

/**
 * SortedProperties.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class SortedProperties extends Properties
{

	private static final long serialVersionUID = -8336236126006306661L;

    /**
     * Returns all the keys
     * @return the keys in the <code>SortedProperties</code>
     */
    public Enumeration<Object> keys ()
    {
        return Collections.<Object>enumeration(_keys);
    }

    /**
     * Sets the property value if value is not null
     * @param key the property key to look for
     * @param value the value to assign to the property
     * @return the value just added if it's not null, or null otherwise
     */
    public synchronized Object setPropertySafely(String key, String value)
    {
        if (value != null) {
            return put(key, value);
        }
        else {
            return null;
        }
    }

    @Override
    public Object put (Object key, Object value)
    {
        _keys.add(key);
        return super.put(key, value);
    }

    private final LinkedHashSet<Object> _keys = new LinkedHashSet<Object>();
}

