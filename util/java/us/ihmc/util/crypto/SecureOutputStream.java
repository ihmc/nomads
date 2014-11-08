/*
 * SecureOutputStream.java
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

import java.io.IOException;
import java.io.OutputStream;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;

public class SecureOutputStream extends OutputStream
{
    public SecureOutputStream (OutputStream os, Cipher c)
    {
        _os = os;
        _cipher = c;
        _blockSize = _cipher.getBlockSize();
        _inBuf = new byte [_blockSize];
        _inBufPos = 0;
        if (_cipher.getAlgorithm().indexOf ("DES") >= 0) {
            _desMode = true;
        }
    }

    public void close()
        throws IOException
    {
        flush();
        _os.close();
    }

    public void flush()
        throws IOException
    {
        if (_inBufPos > 0) {
            byte[] output = encryptBlock();
            _os.write (output);
            _inBufPos = 0;
        }
    }

    public void write (byte[] b)
        throws IOException
    {
        write (b, 0, b.length);
    }

    public void write (byte[] b, int off, int len)
        throws IOException
    {
        int bytesWritten = 0;
        while (bytesWritten < len) {
            int roomAvail = _blockSize - _inBufPos;
            int bytesToWrite = len - bytesWritten;
            if (bytesToWrite > roomAvail) {
                bytesToWrite = roomAvail;
            }
            System.arraycopy (b, off+bytesWritten, _inBuf, _inBufPos, bytesToWrite);
            _inBufPos += bytesToWrite;
            bytesWritten += bytesToWrite;
            if (_inBufPos == _blockSize) {
                byte[] output = encryptBlock();
                _os.write (output);
                _inBufPos = 0;
            }
        }
    }

    public void write (int b)
        throws IOException
    {
        if (_inBufPos < _blockSize) {
            _inBuf[_inBufPos++] = (byte) b;
        }
    }

    protected byte[] encryptBlock()
        throws IOException
    {
        try {
            if (_desMode) {
                while (_inBufPos < _blockSize) {
                    _inBuf[_inBufPos++] = 0;
                }
            }
            return _cipher.doFinal (_inBuf, 0, _inBufPos);
        }
        catch (BadPaddingException e) {
            throw new IOException ("nested exception - " + e);
        }
        catch (IllegalBlockSizeException e) {
            throw new IOException ("nested exception - " + e);
        }
    }

    protected OutputStream _os;
    protected boolean _desMode;
    protected Cipher _cipher;
    protected int _blockSize;
    protected byte[] _inBuf;
    protected int _inBufPos;
}
