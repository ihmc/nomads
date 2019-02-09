package us.ihmc.aci.kernel;

import java.io.InputStream;
import java.io.IOException;

public class ServiceInputStream extends InputStream
{
    public int read (byte b[], int off, int len)
        throws IOException
    {
    	int bytesRead = arrayRead (b, off, len);
        if (bytesRead > 0) {
        	_totBytesRead += bytesRead;
        }
        return bytesRead;
    }

    protected void finalize()
    {
        if (_deleteReader) {
            this.destroy();
        }
    }

    private native int arrayRead (byte b[], int off, int len) throws IOException;
    public native int available() throws IOException;
    public native void close() throws IOException;
    public native int read() throws IOException;
    private native void destroy();
    
    // Method used for picking up total bytes read.
    public long getTotalBytesRead()
    {
        return _totBytesRead;
    }

    protected boolean _deleteReader = false;

    // ----
    // fields used by the JNI native code.
    // DO NOT change their names or types unless you really know what you're doing.
    private int _reader;
    private long _totBytesRead = 0;
    // ----
}
