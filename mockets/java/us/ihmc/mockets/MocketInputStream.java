/*
 * MocketInputStream.java
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
 * @version     $Revision: 1.12 $
 *              $Date: 2015/02/20 23:34:18 $
 */

package us.ihmc.mockets;

import java.io.InputStream;
import java.io.IOException;

public class MocketInputStream extends InputStream
{
    MocketInputStream (StreamMocket mocket)
    {
        init (mocket);
        _streamMocketRef = mocket;
    }

    public native void init (StreamMocket mocket);

    public native int read()
        throws IOException;

    private native int nativeRead (byte[] buf, int off, int len)
        throws IOException;

    /**
     *
     */
    public int read (byte[] buf)
        throws IOException
    {
        if (buf == null) {
            new NullPointerException ("buf cannot be null");
        }

        return nativeRead (buf, 0, buf.length);
    }

    /**
     *
     */
    public int read (byte[] buf, int off, int len)
        throws IOException
    {
        if (buf == null) {
            throw new NullPointerException ("buff cannot be null");
        }

        return nativeRead (buf, off, len);
    }

    public void close()
        throws IOException
    {
        _streamMocketRef.closeSync();
    }

    public StreamMocket getStreamMocket()
    {
        return _streamMocketRef;
    }


    // /////////////////////////////////////////////////////////////////////////
    private StreamMocket _streamMocketRef = null;

    // /////////////////////////////////////////////////////////////////////////
    // this field is used by the JNI code. DO NOT change or modify it unless you know what you're doing.
    private long _mocket = 0;
}
