package us.ihmc.mockets;

import java.io.InputStream;
import java.io.IOException;

/**
 * The MocketInputStream class implements the input stream
 * mechanism for mockets. See java.io.InputStream for
 * more information.
 */
public class MocketInputStream extends InputStream
{
    public int available()
        throws IOException
    {
        return _mocket.bytesAvailable();
    }

    public int read()
        throws IOException
    {
        int value = _mocket.receive (_byteBuf, 0, 1);
        if (value == 1) {
            // Got a byte, so return that
            value = _byteBuf[0];
            if (value < 0) {
                value += 256;
            }
        }
        return value;
    }

    public int read (byte[] buf, int off, int len)
        throws IOException
    {
        return _mocket.receive (buf, off, len);
    }

    MocketInputStream (StreamMocket mocket)
    {
        _mocket = mocket;
        _byteBuf = new byte[1];
    }
    
    private StreamMocket _mocket;
    private byte[] _byteBuf;
}
