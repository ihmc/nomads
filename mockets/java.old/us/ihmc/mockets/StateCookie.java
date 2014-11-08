/**
 * The StateCookie class represents the state cookie included in the INIT-ACK
 * and COOKIE-ECHO chunks of a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class StateCookie
{
    StateCookie (byte[] buf, int off)
    {
        if (buf.length < off + STATE_COOKIE_SIZE)
            throw new RuntimeException ("Input buffer too small to contain a state cookie!");

        _buf = buf;
        _off = off;

        _generationTime        = ByteConverter.from8BytesToLong (_buf, _off);
        _lifeSpan              = ByteConverter.from8BytesToLong (_buf, _off + 8);
        _validation_A          = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 16);
        _validation_Z          = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 20);
        _initialControlTSN_A   = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 24);
        _initialControlTSN_Z   = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 28);
        _initialRelSeqTSN_A    = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 32);
        _initialRelSeqTSN_Z    = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 36);
        _initialUnrelSeqTSN_A  = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 40);
        _initialUnrelSeqTSN_Z  = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 44);
        _initialRelUnseqID_A   = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 48);
        _initialRelUnseqID_Z   = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 52);
        _initialUnrelUnseqID_A = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 56);
        _initialUnrelUnseqID_Z = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 60);
        _port_A                = ByteConverter.from2BytesToUnsignedShortInt (_buf, _off + 64);
        _port_Z                = ByteConverter.from2BytesToUnsignedShortInt (_buf, _off + 66);
        
        /* no implementation of hash at the moment */
    }

    StateCookie (long generationTime, long lifeSpan, 
                 long validation_A, long validation_Z,
                 long initialRelSeqTSN_A, long initialUnrelSeqTSN_A,
                 long initialControlTSN_A, long initialRelUnseqID_A,
                 long initialUnrelUnseqID_A,
                 long initialRelSeqTSN_Z, long initialUnrelSeqTSN_Z,
                 long initialControlTSN_Z, long initialRelUnseqID_Z,
                 long initialUnrelUnseqID_Z,
                 int port_A, int port_Z) 
    {
        _buf = null;
        _off = 0;
        
        _generationTime = generationTime;
        _lifeSpan = lifeSpan;       
            
        if (validation_A < 1 || validation_A > 4294967295L)
            throw new IllegalArgumentException ("Validation from endpoint A must be " +
                                                "a value between 1 and 2^32-1!");
        _validation_A = validation_A;
        
        if (validation_Z < 1 || validation_Z > 4294967295L)
            throw new IllegalArgumentException ("Validation from endpoint Z must be " +
                                                "a value between 1 and 2^32-1!");
        _validation_Z = validation_Z;
        
        if (initialRelSeqTSN_A < 0 || initialRelSeqTSN_A > 4294967295L)
            throw new IllegalArgumentException ("initialRelSeqTSN_A must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialRelSeqTSN_A = initialRelSeqTSN_A;

        if (initialUnrelSeqTSN_A < 0 || initialUnrelSeqTSN_A > 4294967295L)
            throw new IllegalArgumentException ("initialUnrelSeqTSN_A must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialUnrelSeqTSN_A = initialUnrelSeqTSN_A;

        if (initialControlTSN_A < 0 || initialControlTSN_A > 4294967295L)
            throw new IllegalArgumentException ("initialControlTSN_A must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialControlTSN_A = initialControlTSN_A;

        if (initialRelUnseqID_A < 0 || initialRelUnseqID_A > 4294967295L)
            throw new IllegalArgumentException ("initialRelUnseqID_A must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialRelUnseqID_A = initialRelUnseqID_A;
       
        if (initialUnrelUnseqID_A < 0 || initialUnrelUnseqID_A > 4294967295L)
            throw new IllegalArgumentException ("initialUnrelUnseqID_A must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialUnrelUnseqID_A = initialUnrelUnseqID_A;
       
        if (initialRelSeqTSN_Z < 0 || initialRelSeqTSN_Z > 4294967295L)
            throw new IllegalArgumentException ("initialRelSeqTSN_Z must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialRelSeqTSN_Z = initialRelSeqTSN_Z;

        if (initialUnrelSeqTSN_Z < 0 || initialUnrelSeqTSN_Z > 4294967295L)
            throw new IllegalArgumentException ("initialUnrelSeqTSN_Z must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialUnrelSeqTSN_Z = initialUnrelSeqTSN_Z;

        if (initialControlTSN_Z < 0 || initialControlTSN_Z > 4294967295L)
            throw new IllegalArgumentException ("initialControlTSN_Z must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialControlTSN_Z = initialControlTSN_Z;

        if (initialRelUnseqID_Z < 0 || initialRelUnseqID_Z > 4294967295L)
            throw new IllegalArgumentException ("initialRelUnseqID_Z must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialRelUnseqID_Z = initialRelUnseqID_Z;
        
        if (initialUnrelUnseqID_Z < 0 || initialUnrelUnseqID_Z > 4294967295L)
            throw new IllegalArgumentException ("initialUnrelUnseqID_Z must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialUnrelUnseqID_Z = initialUnrelUnseqID_Z;
       
        if (port_A < 0 || port_A > 65535)
            throw new IllegalArgumentException ("Port used by endpoint A must be " + 
                                                "a value between 0 and 2^16-1!");
        _port_A = port_A;
        
        if (port_Z < 0 || port_Z > 65535)
            throw new IllegalArgumentException ("Port used by endpoint Z must be " + 
                                                "a value between 0 and 2^16-1!");
        _port_Z = port_Z;

        /* no implementation of hash at the moment */
    }

    void write (byte[] buf, int off) 
    {
        if (buf == null)
            throw new IllegalArgumentException ("Invalid argument!");

        if (buf.length < off + STATE_COOKIE_SIZE)
            throw new RuntimeException ("Not enough space in destination buffer!");

        if (_buf != null) {
            // _buf != null is a perfectly fine condition, as we want to be
            // able to copy a state cookie from an incoming INIT-ACK chunk
            // to an outgoing COOKIE-ECHO chunk
            System.arraycopy (_buf, _off, buf, off, STATE_COOKIE_SIZE);
        } else {
            ByteConverter.fromLongTo8Bytes (_generationTime, buf, off);
            ByteConverter.fromLongTo8Bytes (_lifeSpan, buf, off + 8);
            ByteConverter.fromUnsignedIntTo4Bytes (_validation_A, buf, off + 16);
            ByteConverter.fromUnsignedIntTo4Bytes (_validation_Z, buf, off + 20);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialControlTSN_A,   buf, off + 24);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialControlTSN_Z,   buf, off + 28);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialRelSeqTSN_A,    buf, off + 32);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialRelSeqTSN_Z,    buf, off + 36);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialUnrelSeqTSN_A,  buf, off + 40);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialUnrelSeqTSN_Z,  buf, off + 44);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialRelUnseqID_A,   buf, off + 48);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialRelUnseqID_Z,   buf, off + 52);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialUnrelUnseqID_A, buf, off + 56);
            ByteConverter.fromUnsignedIntTo4Bytes (_initialUnrelUnseqID_Z, buf, off + 60);
            ByteConverter.fromUnsignedShortIntTo2Bytes (_port_A, buf, off + 64);
            ByteConverter.fromUnsignedShortIntTo2Bytes (_port_Z, buf, off + 66);
        }

        /* no implementation of hash at the moment */
    }

    long getAValidation() 
    {
        return _validation_A;
    }
    
    long getZValidation() 
    {
        return _validation_Z;
    }
    
    long getAInitialRelSeqTSN() 
    {
        return _initialRelSeqTSN_A;
    }

    long getAInitialUnrelSeqTSN() 
    {
        return _initialUnrelSeqTSN_A;
    }

    long getAInitialControlTSN() 
    {
        return _initialControlTSN_A;
    }
    
    long getAInitialRelUnseqID() 
    {
        return _initialRelUnseqID_A;
    }
    
    long getAInitialUnrelUnseqID() 
    {
        return _initialUnrelUnseqID_A;
    }
    
    long getZInitialRelSeqTSN() 
    {
        return _initialRelSeqTSN_Z;
    }

    long getZInitialUnrelSeqTSN() 
    {
        return _initialUnrelSeqTSN_Z;
    }

    long getZInitialControlTSN() 
    {
        return _initialControlTSN_Z;
    }
    
    long getZInitialRelUnseqID() 
    {
        return _initialRelUnseqID_Z;
    }
    
    long getZInitialUnrelUnseqID() 
    {
        return _initialUnrelUnseqID_Z;
    }
    
    int getAPort() 
    {
        return _port_A;
    }

    int getZPort() 
    {
        return _port_Z;
    }

    boolean isEqual (StateCookie cookie) 
    {
        return (_generationTime        == cookie._generationTime &&
                _lifeSpan              == cookie._lifeSpan &&
                _validation_A          == cookie._validation_A && 
                _validation_Z          == cookie._validation_Z &&
                _initialRelSeqTSN_A    == cookie._initialRelSeqTSN_A &&
                _initialUnrelSeqTSN_A  == cookie._initialUnrelSeqTSN_A &&
                _initialControlTSN_A   == cookie._initialControlTSN_A &&
                _initialRelUnseqID_A   == cookie._initialRelUnseqID_A &&
                _initialUnrelUnseqID_A == cookie._initialUnrelUnseqID_A &&
                _initialRelSeqTSN_Z    == cookie._initialRelSeqTSN_Z &&
                _initialUnrelSeqTSN_Z  == cookie._initialUnrelSeqTSN_Z &&
                _initialControlTSN_Z   == cookie._initialControlTSN_Z &&
                _initialRelUnseqID_Z   == cookie._initialRelUnseqID_Z &&
                _initialUnrelUnseqID_Z == cookie._initialUnrelUnseqID_Z &&
                _port_A                == cookie._port_A &&
                _port_Z                == cookie._port_Z);
    }

    /*  8 bytes for the cookie generation timestamp, 
     *  8 bytes for the cookie lifespan,
     *  4 bytes for the local validation tag, 
     *  4 bytes for the remote validation tag,
     * 20 bytes for the local transmission sequence numbers, 
     * 20 bytes for the remote transmission sequence numbers, 
     *  2 bytes for the local port,
     *  2 bytes for the remote port */
    //  in future we will probably add 20? bytes for the HMAC-SHA1 hash.
    public static final int STATE_COOKIE_SIZE = 68;
    
    private byte[] _buf;
    private int _off;
    private int _len;
    private long _generationTime;
    private long _lifeSpan;
    private long _validation_A;
    private long _validation_Z;
    private long _initialRelSeqTSN_A;
    private long _initialUnrelSeqTSN_A;
    private long _initialControlTSN_A;
    private long _initialRelUnseqID_A;
    private long _initialUnrelUnseqID_A;
    private long _initialRelSeqTSN_Z;
    private long _initialUnrelSeqTSN_Z;
    private long _initialControlTSN_Z;
    private long _initialRelUnseqID_Z;
    private long _initialUnrelUnseqID_Z;
    private int _port_A;
    private int _port_Z;

    /* no implementation of hash at the moment */
}

/*
 * vim: et ts=4 sw=4
 */

