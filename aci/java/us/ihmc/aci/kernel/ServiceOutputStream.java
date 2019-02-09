package us.ihmc.aci.kernel;

import java.io.OutputStream;
import java.io.IOException;

public class ServiceOutputStream extends OutputStream
{
    public void write (byte b[], int off, int len)
        throws IOException
    {
    	arrayWrite (b, off, len);
        _totBytesWritten += len;
    }

    protected void finalize()
    {
        if (_deleteWriter) {
            this.destroy();
        }
    }

    public native void write (int b) throws IOException;
    public native void flush() throws IOException;
    public native void close() throws IOException;
    public native void arrayWrite (byte b[], int off, int len) throws IOException;
    private native void destroy();

    // Method used for picking up total bytes written.
    public long getTotalBytesWritten()
    {
        return _totBytesWritten;
    }
    
    protected boolean _deleteWriter = false;

    // ----
    // fields used by the JNI native code.
    // DO NOT change their names or types unless you really know what you're doing.
    private int _writer;
    private long _totBytesWritten = 0;
    // ----
}
