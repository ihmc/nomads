package us.ihmc.mockets;

import java.io.IOException;
import java.io.OutputStream;

/**
 * The MocketOutputStream class implements the output stream
 * mechanism for mockets. See java.io.OutputStream for more
 * information.
 */
public class MocketOutputStream extends OutputStream
{
    public void write (int b)
        throws IOException
    {
        _byteBuf[0] = (byte) b;
        _mocket.send (_byteBuf, 0, 1);
    }

    public void write (byte[] buf)
        throws IOException
    {
        _mocket.send (buf, 0, buf.length);
    }

    MocketOutputStream (StreamMocket mocket)
    {
        _mocket = mocket;
        _byteBuf = new byte[1];
    }
    
    private StreamMocket _mocket;
    private byte[] _byteBuf;
}
