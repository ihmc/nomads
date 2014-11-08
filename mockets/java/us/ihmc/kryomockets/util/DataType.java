/*
 * DataType.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets.util;

/**
 * DataType
 *
 * Enum <code>DataType</code> handles the different kinds of Data.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public enum  DataType
{
    TEXT(0),
    AUDIO(1);

    private final int _code;

    DataType (int code)
    {
        _code = code;
    }

    public int code ()
    {
        return _code;
    }
}
