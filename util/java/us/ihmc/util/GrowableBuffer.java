/*
 * GrowableBuffer.java
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

package us.ihmc.util;

/**
 * GrowableBuffer is a utility class that provides a growable buffer.
 * 
 * The class maintains an internal buffer that grows dynamically as needed
 * when the append() method is invoked.
 * 
 * Once all the data has been placed into the buffer, the internal buffer
 * may be retrieved by using the getBuffer() method. If a copy of the buffer
 * is needed, the getBufferCopy() method can be used.
 * 
 * Note that the buffer returned by getBuffer() may be larger than the actual
 * data that was placed into the buffer. Therefore, to get a buffer that is
 * exactly the size of the data, use getBufferCopy().
 * 
 * The growth increment of the buffer can be specified at the time the buffer
 * is constructed.
 */
public class GrowableBuffer
{
    /**
     * Construct a buffer with a default initialize size (of 1024) and a default
     * increment size (of 1024).
     */
    public GrowableBuffer()
    {
        init (1024, 1024);
    }

    /**
     * Construct a buffer with the specified initial size and a default increment
     * size (of 1024).
     */
    public GrowableBuffer (int initSize)
    {
        init (initSize, 1024);
    }
    
    /**
     * Construct a buffer with the specified initial size and the specified
     * increment size (of 1024).
     */
    public GrowableBuffer (int initSize, int incrementSize)
    {
        init (initSize, incrementSize);
    }
    
    public void append (byte buffer[])
    {
        if (_remainingBufferSpace < buffer.length) {
            growBuffer (buffer.length);
        }
        System.arraycopy (buffer, 0, _buffer, _totalBufferSize - _remainingBufferSpace, buffer.length);
        _remainingBufferSpace -= buffer.length;
    }

    public void append (byte buffer[], int pos, int len)
    {
        if (_remainingBufferSpace < len) {
            growBuffer (len);
        }
        System.arraycopy (buffer, pos, _buffer, _totalBufferSize - _remainingBufferSpace, len);
        _remainingBufferSpace -= len;
    }

    public byte[] getBuffer()
    {
        return _buffer;
    }
    
    public byte[] getBufferCopy()
    {
        byte copiedBuffer[] = new byte [_totalBufferSize - _remainingBufferSpace];
        System.arraycopy (_buffer, 0, copiedBuffer, 0, _totalBufferSize - _remainingBufferSpace);
        return copiedBuffer;
    }

    public void clear()
    {
        for (int i = 0; i < _totalBufferSize - _remainingBufferSpace; i++) {
            _buffer[i] = 0;
        }
        _remainingBufferSpace = _totalBufferSize;
    }

    private void init (int size, int increment)
    {
        _buffer = new byte[size];
        _totalBufferSize = size;
        _bufferSizeIncrement = increment;
        _remainingBufferSpace = size;
    }

    private void growBuffer (int currentRequirement)
    {
        int growthNeeded = currentRequirement - _remainingBufferSpace;
        if (growthNeeded < _bufferSizeIncrement) {
            growthNeeded = _bufferSizeIncrement;
        }

        int newBufferSize = _totalBufferSize + growthNeeded;
        byte newBuffer[] = new byte[newBufferSize];
        System.arraycopy (_buffer, 0, newBuffer, 0, _totalBufferSize - _remainingBufferSpace);

        _buffer = newBuffer;
        _totalBufferSize = newBufferSize;
        _remainingBufferSpace += growthNeeded;
    }

    private byte _buffer[];
    int _totalBufferSize;
    int _bufferSizeIncrement;
    int _remainingBufferSpace;
}
