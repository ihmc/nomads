/*
 * SecureInputStream.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 */

package us.ihmc.util.crypto;

import java.io.DataInputStream;
import java.io.EOFException;
import java.io.InputStream;
import java.io.IOException;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;

public class SecureInputStream extends InputStream
{
    public SecureInputStream (InputStream is, Cipher c)
    {
        _dis = new DataInputStream (is);
        _cipher = c;
        _blockSize = _cipher.getBlockSize();
        _outBuf = null;
        _outBufPos = 0;
    }

    public int available()
        throws IOException
    {
        if (_outBuf != null) {
            return _outBuf.length - _outBufPos;
        }
        return 0;
    }

    public void close()
        throws IOException
    {
        _dis.close();
    }

    public void mark (int readlimit)
    {
        // Mark is not supported
    }

    public boolean markSupported()
    {
        return false;
    }

    public int read()
        throws IOException
    {
        if ((_outBuf != null) && (_outBufPos < _outBuf.length)) {
            return _outBuf[_outBufPos++];
        }
        else {
            if (decryptBlock() < 0) {
                // EOF reached
                return -1;
            }
            // Now that another block has been decrypted and buffered, simply call read() again
            return read();
        }
    }

    public int read (byte[] b)
        throws IOException
    {
        return read (b, 0, b.length);
    }

    public int read (byte[] b, int off, int len)
        throws IOException
    {
        if ((_outBuf != null) && (_outBufPos < _outBuf.length)) {
            int bytesToRead = _outBuf.length - _outBufPos;
            if (bytesToRead > len) {
                bytesToRead = len;
            }
            System.arraycopy (_outBuf, _outBufPos, b, off, bytesToRead);
            _outBufPos += bytesToRead;
            return bytesToRead;
        }
        else {
            if (decryptBlock() < 0) {
                // EOF reached
                return -1;
            }
            // Now that another block has been decrypted and buffered, simply call read() again
            return read (b, off, len);
        }
    }

    public void reset()
        throws IOException
    {
        throw new IOException ("mark/reset not supported");
    }

    protected int decryptBlock()
        throws IOException
    {
        try {
            byte[] input = new byte [_blockSize];
            _dis.readFully (input);
            _outBuf = _cipher.doFinal (input);
            _outBufPos = 0;
        }
        catch (EOFException e) {
            return -1;
        }
        catch (BadPaddingException e) {
            throw new IOException ("nested exception - " + e);
        }
        catch (IllegalBlockSizeException e) 
        {
            throw new IOException ("nested exception - " + e);
        }
        return 0;
    }

    protected DataInputStream _dis;
    protected Cipher _cipher;
    protected int _blockSize;
    protected byte[] _outBuf;
    protected int _outBufPos;
}
