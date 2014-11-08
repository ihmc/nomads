/*
 * Data.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets.util;

/**
 * Data.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Data
{
    public Data()
    {

    }

    public Data(String id)
    {
        _id = id;
    }

    public String getId()
    {
        return _id;
    }

    public void setType(DataType type)
    {
        _type = type;
    }

    public DataType getType()
    {
        return _type;
    }

    public boolean isChunked()
    {
        return _isChunked;
    }

    public void setChunked(boolean value)
    {
        _isChunked = true;
    }

    public void setChunkNumber(int number)
    {
        _chunkNumber = number;
    }

    public int getChunkNumber()
    {
        return _chunkNumber;
    }

    public int getTotalChunks ()
    {
        return _totalChunks;
    }

    public void setTotalChunks (int totalChunks)
    {
        this._totalChunks = totalChunks;
    }

    public int getTotalLength ()
    {
        return _totalLength;
    }

    public void setTotalLength (int totalLength)
    {
        this._totalLength = totalLength;
    }

    public void setSenderUUID (String nodeUUID)
    {
        _senderUUID = nodeUUID;
    }

    public String getSenderUUID ()
    {
        return _senderUUID;
    }

    public void setText (String text)
    {
        _text = text;
    }

    public void setRawData (byte[] data)
    {
        _rawData = data;
    }

    public String getText ()
    {
        return _text;
    }

    public byte[] getRawData ()
    {
        return _rawData;
    }

    private String _id;
    private String _senderUUID;
    private String _text;
    private boolean _isChunked = false;
    private int _chunkNumber = -1;
    private int _totalChunks = -1;
    private int _totalLength = -1;
    private byte[] _rawData;
    private DataType _type;
}
