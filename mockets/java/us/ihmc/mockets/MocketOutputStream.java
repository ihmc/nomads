/*
 * MocketOutputStream.java
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 * 
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 * 
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * @version     $Revision: 1.8 $
 *              $Date: 2014/10/29 23:05:27 $
 */

package us.ihmc.mockets;

import java.io.IOException;
import java.io.OutputStream;

public class MocketOutputStream extends OutputStream
{
    MocketOutputStream (StreamMocket mocket)
    {
        init (mocket);
        _streamMocketRef = mocket;
    }

    private native void init (StreamMocket mocket);

    public native void write (int b)
        throws IOException;

    public native void flush()
        throws IOException;

    public native void close()
        throws IOException;

    private native void nativeWrite (byte[] buf, int off, int len)
        throws IOException;

    private native void dispose();

    /**
     *
     */
    public void write (byte[] buf)
       throws IOException
    {
        if (buf == null) {
            throw new NullPointerException ("buf cannot be null.");
        }

        this.write (buf, 0, buf.length);
    }

    /**
     *
     */
    public void write (byte[] buf, int off, int len)
       throws IOException
    {
        if (buf == null) {
            throw new NullPointerException ("buf cannot be null.");
        }

        this.nativeWrite (buf, off, len);
    }

    public StreamMocket getStreamMocket()
    {
        return _streamMocketRef;
    }

    /**
     *
     */
    protected void finalize()
    {
        dispose();
    }

    // /////////////////////////////////////////////////////////////////////////
    private StreamMocket _streamMocketRef = null;

    // /////////////////////////////////////////////////////////////////////////
    // this field is used by the JNI code. DO NOT change or modify it unless you know what you're doing.
    private long _mocket = 0;
}
