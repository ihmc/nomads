package us.ihmc.aci.util.chunking;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public enum Dimension implements us.ihmc.chunking.Dimension {
    X ((byte)0),
    Y ((byte)1),
    T ((byte)3);

    private byte _value;
    
    Dimension (byte value)
    {
        _value = value;
    }

    static Dimension read(CommHelper ch) throws CommException, ProtocolException
    {
        final byte value = ch.read8();
        switch (value) {
            case 0: return X;
            case 1: return Y;
            case 3: return T;
            default:
                throw new IllegalArgumentException ("Unknown dimension of value " + value);
        }
    }

    void write(CommHelper ch) throws CommException
    {
        ch.write8(_value);
    }
}
