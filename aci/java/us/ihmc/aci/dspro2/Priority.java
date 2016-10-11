package us.ihmc.aci.dspro2;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public enum Priority
{
    
    HIGHEST ((short) 5),
    HIGH ((short) 4),
    MEDIUM ((short) 3),
    LOW ((short) 2),
    LOWEST ((short) 1);

    private final short _val;

    Priority (short val)
    { 
        _val = val;
    }

    short getVal()
    {
        return _val;
    }

    @Override
    public String toString()
    {
        return Short.toString (_val);
    }
}
