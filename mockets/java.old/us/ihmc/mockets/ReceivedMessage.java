/**
 * The ReceivedMessage class represents a ULP message received from a remote endpoint.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.lang.Math;
import java.util.Enumeration;
import java.util.Vector;
import java.util.logging.Logger;


class ReceivedMessage
{
    ReceivedMessage (DataBuffer db)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        _v = new Vector();
        _v.add (db);
        
        _size = db.getSize();
    }

    ReceivedMessage (Vector dbv)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        _v = dbv;

        _size = 0;
        for (Enumeration e = _v.elements(); e.hasMoreElements();) {
            _size += ((DataBuffer)e.nextElement()).getSize();
        }
    }

    int getData (byte[] buf, int off, int len)
        throws NotEnoughSizeException 
    {
        int toCopy = Math.min (len, _size);
        int copied = 0;
                
        while (copied < toCopy) {
            for (Enumeration e = _v.elements(); e.hasMoreElements();) {
                DataBuffer db = (DataBuffer)e.nextElement();
                int size = Math.min (db.getSize(), toCopy - copied);
                db.getData (buf, off, size);
                copied += size;
                off += size;
            }
        }
        
        if (len < _size) {
            _logger.warning ("output buffer too small: discarded " + 
                             (_size - toCopy) + " bytes."); 
            throw new NotEnoughSizeException (len, _size);
        }
        
        return copied;
    }

    int getSize()
    {
        return _size;
    }

    private Logger _logger;
    private Vector _v;
    private int _size;
}
/*
 * vim: et ts=4 sw=4
 */
