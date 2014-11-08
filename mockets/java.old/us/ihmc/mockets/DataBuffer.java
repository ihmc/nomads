/**
 * The DataBuffer class represents the data buffer contained in a DATA chunk.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.lang.Math;
import java.util.logging.Logger;


class DataBuffer
{
    DataBuffer (byte[] buf, int off, int len)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        _buf = buf;
        _offset = off;
        _size = len;
    }

    int getData (byte[] buf, int off, int len)
        throws NotEnoughSizeException 
    {
        int toCopy = Math.min (len, _size);
        
        System.arraycopy (_buf, _offset, buf, off, toCopy);

        if (len < _size) {
            _logger.warning ("output buffer too small: discarded " + 
                             (_size - toCopy) + " bytes."); 
            throw new NotEnoughSizeException (len, _size);
        }
        
        return toCopy;
    }

    int getSize()
    {
        return _size;
    }

    private Logger _logger;
    private byte[] _buf;
    private int _size;
    private int _offset;
}
/*
 * vim: et ts=4 sw=4
 */
