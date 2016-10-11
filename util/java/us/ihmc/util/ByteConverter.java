/*
 * ByteConverter.java
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
 *
 * Utility class to help with conversion from primitive types into 
 * corresponding byte-array representations.
 */

package us.ihmc.util;

public class ByteConverter
{
    /**
     * Convert an unsigned byte value into a byte.
     * Although the input parameter is of type short, the value should range
     * between 0 and 255.
     * 
     * @param value     the value to be converted
     * @param bytes     the byte array into which the value will be stored
     * @param off       the starting offset into the byte array
     */
    public static void fromUnsignedByteTo1Byte (short value, byte[] bytes, int off)
    {
        bytes[off] = (byte) (value & 0xFF);
    }

    /**
     * Convert an unsigned byte value into a byte.
     * Although the input parameter is of type short, the value should range
     * between 0 and 255.
     * 
     * @param value     the value to be converted
     * @param bytes     the byte array into which the value will be stored
     */
    public static void fromUnsignedByteTo1Byte (short value, byte[] bytes)
    {
        fromUnsignedByteTo1Byte (value, bytes, 0);
    }
    
    /**
     * Convert a byte representation (a value that ranges from -128 to 127)
     * back into an unsigned byte value (that ranges from 0 to 255).
     * 
     * @param b    the byte to be converted
     * 
     * @return     the converted value which will range from 0 to 255
     */
    public static short from1ByteToUnsignedByte (byte b)
    {
        short value = b;
        if (value < 0) {
            value += 256;
        }
        return value;
    }

    /**
     * Convert a byte representation (a value that ranges from -128 to 127)
     * stored in a byte array back into an unsigned byte value
     * (that ranges from 0 to 255).
     * 
     * @param bytes    the byte array that contains the value to be converted
     * @param off     the starting offset into the byte array
     * 
     * @return     the converted value which will range from 0 to 255
     */
    public static short from1ByteToUnsignedByte (byte[] bytes, int off)
    {
        return from1ByteToUnsignedByte (bytes[off]);
    }
    
    /**
     * Convert a byte representation (a value that ranges from -128 to 127)
     * stored in a byte array back into an unsigned byte value
     * (that ranges from 0 to 255).
     * 
     * @param bytes    the byte array that contains the value to be converted
     * 
     * @return     the converted value which will range from 0 to 255
     */
    public static short from1ByteToUnsignedByte (byte[] bytes)
    {
        return from1ByteToUnsignedByte (bytes[0]);
    }

    /**
     * Convert an unsigned short into into a sequence of two bytes.
     * Although the input parameter is of type int, the value should be between
     * 0 and 65535.
     * 
     * @param value     the value to be converted
     * @param bytes     the byte array into which the value will be stored
     * @param off       the starting offset into the byte array
     */
    public static void fromUnsignedShortIntTo2Bytes (int value, byte[] bytes, int off)
    {
        bytes[off] = (byte) ((value >> 8) & 0xFF);
        bytes[off+1] = (byte) (value & 0xFF);
    }
    
    /**
     * Convert an unsigned short into into a sequence of two bytes.
     * Although the input parameter is of type int, the value should be between
     * 0 and 65535.
     * 
     * @param value     the value to be converted
     * @param bytes     the 2 element byte array into which the value will be stored
     */
    public static void fromUnsignedShortIntTo2Bytes (int value, byte[] bytes)
    {
        fromUnsignedShortIntTo2Bytes (value, bytes, 0);
    }

    /**
     * Convert a byte array representation of a 2-byte unsigned short int
     * back into the integer representation.
     * 
     * @param bytes   the byte array that contains the representation
     *                of the unsigned short int
     * @param off     the starting offset into the byte array
     * 
     * @return the converted int value which will be in the range between 0 and 65535
     */
    public static int from2BytesToUnsignedShortInt (byte[] bytes, int off)
    {
        int value = 0;
        
        // Since byte is signed (-128 to 127), convert it to unsigned if necessary
        int byteAsInt;
        byteAsInt = bytes[off];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 8);
        byteAsInt = bytes[off+1];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= byteAsInt;

        return value;
    }
    
    /**
     * Convert a byte array representation of a 2-byte unsigned short int
     * back into the integer representation.
     * 
     * @param bytes   the 2 element byte array that contains the representation
     *                of the unsigned short int
     * 
     * @return the converted int value which will be in the range between 0 and 65535
     */
    public static int from2BytesToUnsignedShortInt (byte[] bytes)
    {
        return from2BytesToUnsignedShortInt (bytes, 0);
    }

    /**
     * Converts a unsigned int to a 4 bytes array
     *
     * @param value unsigned int to convert
     * @param bytes 4 bytes array
     * @param off offset
     */
    public static void fromUnsignedIntTo4Bytes (long value, byte[] bytes, int off)
    {
        bytes[off] = (byte) ((value >> 24) & 0xFF);
        bytes[off+1] = (byte) ((value >> 16) & 0xFF);
        bytes[off+2] = (byte) ((value >> 8) & 0xFF);
        bytes[off+3] = (byte) (value & 0xFF);
    }

    /**
     * Convert a byte array representation of a 4-byte unsigned int
     * back into the unsigned integer representation.
     *
     * @param bytes byte array to read
     * @param off offset
     *
     * @return the converted unsigned int value
     */
    public static long from4BytesToUnsignedInt (byte[] bytes, int off)
    {
        long value = 0;
        long byteValue;
        byteValue = bytes[off];
        if (byteValue < 0) {
            byteValue += 256;
        }
        value |= (byteValue << 24);
        
        byteValue = bytes[off+1];
        if (byteValue < 0) {
            byteValue += 256;
        }
        value |= (byteValue << 16);

        byteValue = bytes[off+2];
        if (byteValue < 0) {
            byteValue += 256;
        }
        value |= (byteValue << 8);
        
        byteValue = bytes[off+3];
        if (byteValue < 0) {
            byteValue += 256;
        }
        value |= byteValue;

        return value;
    }

    /**
     * Convert a byte array representation of a 4-byte int
     * back into the integer representation.
     *
     * @param bytes byte array to read
     * @param off offset
     *
     * @return the converted int value
     */
    public static long from4BytesToSignedInt (byte[] bytes, int off)
    {
        long value = 0;
        long byteValue;
        byteValue = bytes[off];

        value |= (byteValue << 24);

        byteValue = bytes[off+1];
        if (byteValue < 0) {
            byteValue += 256;
        }
        value |= (byteValue << 16);

        byteValue = bytes[off+2];
        if (byteValue < 0) {
            byteValue += 256;
        }
        value |= (byteValue << 8);

        byteValue = bytes[off+3];
        if (byteValue < 0) {
            byteValue += 256;
        }
        value |= byteValue;

        return value;
    }

    /**
     * Convert a long into a sequence of 8 bytes.
     *
     * @param value     the value to be converted
     * @param bytes     the 8 element byte array into which the value will be stored
     * @param off       offset
     */
    public static void fromLongTo8Bytes (long value, byte[] bytes, int off)
    {
        /* bigendian */
        bytes[off]   = (byte) ((value >> 56) & 0xFF);
        bytes[off+1] = (byte) ((value >> 48) & 0xFF);
        bytes[off+2] = (byte) ((value >> 40) & 0xFF);
        bytes[off+3] = (byte) ((value >> 32) & 0xFF);
        bytes[off+4] = (byte) ((value >> 24) & 0xFF);
        bytes[off+5] = (byte) ((value >> 16) & 0xFF);
        bytes[off+6] = (byte) ((value >>  8) & 0xFF);
        bytes[off+7] = (byte) ((value)       & 0xFF);
    }
    
    /**
     * Convert a long into a sequence of 8 bytes.
     * 
     * @param value     the value to be converted
     * @param bytes     the 8 element byte array into which the value will be stored
     */
    public static void fromLongTo8Bytes (long value, byte[] bytes)
    {
        fromLongTo8Bytes (value, bytes, 0);
    }

    /**
     * Convert a byte array representation of a 8-byte long
     * back into the long representation.
     *
     * @param bytes byte array to read
     * @param off offset
     *
     * @return the converted long value
     */
    public static long from8BytesToLong (byte[] bytes, int off)
    {
        long value = 0;
        long byteAsInt;

        if (bytes.length < 8)
            throw new IllegalArgumentException ("The size of the input array " +
                                                "must be at least 8 bytes.");
        
        byteAsInt = bytes[off];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 56);
        
        byteAsInt = bytes[off+1];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 48);
        
        byteAsInt = bytes[off+2];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }        
        value |= (byteAsInt << 40);
        
        byteAsInt = bytes[off+3];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 32);
        
        byteAsInt = bytes[off+4];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 24);
        
        byteAsInt = bytes[off+5];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 16);
        
        byteAsInt = bytes[off+6];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 8);
        
        byteAsInt = bytes[off+7];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= byteAsInt;

        return value;
    }

    /**
     * Convert a byte array representation of a 8-byte signed long
     * back into the signed long representation.
     *
     * @param bytes byte array to read
     * @param off offset
     *
     * @return the converted signed long value
     */
    public static long from8BytesToSignedLong (byte[] bytes, int off)
    {
        long value = 0;
        long byteAsInt;

        if (bytes.length < 8)
            throw new IllegalArgumentException ("The size of the input array " +
                                                "must be at least 8 bytes.");

        byteAsInt = bytes[off];

        value |= (byteAsInt << 56);

        byteAsInt = bytes[off+1];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 48);

        byteAsInt = bytes[off+2];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 40);

        byteAsInt = bytes[off+3];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 32);

        byteAsInt = bytes[off+4];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 24);

        byteAsInt = bytes[off+5];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 16);

        byteAsInt = bytes[off+6];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= (byteAsInt << 8);

        byteAsInt = bytes[off+7];
        if (byteAsInt < 0) {
            byteAsInt += 256;
        }
        value |= byteAsInt;

        return value;
    }

    /**
     * Convert a byte array representation of a 8-byte long
     * back into the long representation.
     *
     * @param bytes byte array to read
     *
     * @return the converted long value
     */
    public static long from8BytesToLong (byte[] bytes)
    {
        return from8BytesToLong(bytes, 0);
    }

    /**
     * Swap the bytes so that they are in reverse order.<p>
     * Useful for endian conversion (big to little and vice-versa).<p>
     * Handles arrays of size 2, 4, or 8
     * 
     * @param bytes - the array of bytes that need to be swapped
     * @param off - the offset into the array where swapping should start
     * @param len - the number of bytes that should be swapped (2, 4, or 8)
     */
    public static void swapBytes (byte[] bytes, int off, int len)
    {
        switch (len) {
            case 2:
            {
                byte b = bytes[off+0];
                bytes[off+0] = bytes[off+1];
                bytes[off+1] = b;
                return;
            }

            case 4:
            {
                byte b = bytes[off+0];
                bytes[off+0] = bytes[off+3];
                bytes[off+3] =  b;
                b = bytes[off+1];
                bytes[off+1] = bytes[off+2];
                bytes[off+2] = b;
                return;
            }

            case 8:
            {
                byte b = bytes[off+0];
                bytes[off+0] = bytes[off+7];
                bytes[off+7] = b;
                b = bytes[off+1];
                bytes[off+1] = bytes[off+6];
                bytes[off+6] = b;
                b = bytes[off+2];
                bytes[off+2] = bytes[off+5];
                bytes[off+5] = b;
                b = bytes[off+3];
                bytes[off+3] = bytes[off+4];
                bytes[off+4] = b;
                return;
            }
        }
    }
}
