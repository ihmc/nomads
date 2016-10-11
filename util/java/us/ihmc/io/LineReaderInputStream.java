/*
 * LineReaderInputStream.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

package us.ihmc.io;

import java.io.IOException;
import java.io.InputStream;

/**
 * LineReaderInputStream.java
 */
public class LineReaderInputStream extends InputStream
{
    /**
     * Constructor for <code>LineReaderInputStream</code>
     * @param is <code>InputStream</code> instance
     */
    public LineReaderInputStream (InputStream is)
    {
        _is = is;
        _bufferedByte = 0;
    }

    /**
     * Tests if this input stream supports the mark and reset methods
     * @return always false
     */
    public boolean markSupported ()
    {
        return false;
    }

    /**
     * Marks the current position in this input stream
     * @param readlimit the maximum limit of bytes that can be read before the mark position becomes invalid
     */
    public void mark (int readlimit)
            throws RuntimeException
    {
    	throw new RuntimeException ("mark() operation is not supported in LinReaderInputStream");
    }

    /**
     * Closes this input stream and releases any system resources associated with the stream
     * @throws IOException if an I/O error occurs
     */
    public void close ()
            throws IOException
    {
        _is.close();
    }

    /**
     * Repositions this stream to the position at the time the mark method was last called on this input stream
     * @throws IOException if this stream has not been marked or if the mark has been invalidated
     */
    public void reset ()
            throws IOException
    {
        throw new IOException ("mark() operation is not supported in LinReaderInputStream");
    }

    /**
     * Skips over and discards n bytes of data from this input stream
     * @param n the number of bytes to be skipped
     * @return the actual number of bytes skipped
     * @throws IOException if an I/O error occurs
     */
    public long skip (long n)
            throws IOException
    {
        if (_bufferEmpty) {
            return _is.skip (n);
        }
        else {
		    _bufferEmpty = true;
			_bufferedByte = 0;
            return _is.skip (n-1);
        }
    }

    /**
     * Returns the number of bytes that can be read (or skipped over) from this input stream without blocking by the
     * next caller of a method for this input stream. The next caller might be the same thread or another thread
     * @return the number of bytes that can be read from this input stream without blocking
     * @throws IOException if an I/O error occurs
     */
    public int available ()
            throws IOException
    {
        if (_bufferEmpty) {
            return _is.available();
        }
        else {
            return _is.available() + 1;
        }
    }

    /**
     * Reads from the <code>InputStream</code> until the characters \r\n are found
     * @return the line just read
     * @throws IOException if an I/O error occurs
     */
    public String readLine ()
            throws IOException
    {
        byte[] buf = new byte[512];
        int index = 0;
        int count = 0;
        synchronized (_is) {
            while (true) {
                int b = read();
                if (b < 0) {
                    throw new IOException ("EOF reached before line terminator");
                }
                buf[index] = (byte) b;

                if (buf[index] == '\r') {
                    // Found the end of the line
                    count = index;

                    // Check if there is a \n also
                    
                    int nextByte = read();
                    if (nextByte < 0) {
                    	break;
                    }
                    else if (nextByte != '\n') {
                        // We overread a byte - put it back
                        putBackByte ((byte)nextByte);
                        break;
                    }
                    else {
                        // We have received a \r and a \n, so drop out
                        break;
                    }
                }
                else if (buf[index] == '\n') {
                    // We have received the end of a line
                    count = index;
                    break;
                }
                index++;
                if (index == buf.length) {
                    // Need to grow the buffer
                    byte[] newBuf = new byte[buf.length + 512];
                    System.arraycopy(buf, 0, newBuf, 0, buf.length);
                    buf = newBuf;
                }
            }
        }
        return new String (buf, 0, count);
    }

    /**
     * Reads the next byte of data from the input stream. The value byte is returned as an int in the range 0 to 255.
     * If no byte is available because the end of the stream has been reached, the value -1 is returned. This method
     * blocks until input data is available, the end of the stream is detected, or an exception is thrown
     * @return the next byte of data, or -1 if the end of the stream is reached
     * @throws IOException if an I/O error occurs
     */
    public int read ()
            throws IOException
    {
        int value;
        if (_bufferEmpty) {
            value = _is.read();
            return value;        	
        }
        else {
        	value = _bufferedByte;
        	_bufferedByte = 0;
        	_bufferEmpty = true;
        	return value;
        }
    }

    /**
     * Set the cursor back to the byte b
     * @param b byte to shift to
     */
    private void putBackByte (byte b)
    {
    	if (_bufferEmpty) {
            _bufferedByte = b;
            _bufferEmpty = false;
    	}
    	else {
    		throw new RuntimeException ("trying to putBackByte() while there is already a byte in the buffer");
    	}
    }

    private final InputStream _is;
    private boolean _bufferEmpty = true;
    private byte _bufferedByte = 0;

}
