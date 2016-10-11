/*
 * ByteArray.java
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

package us.ihmc.util;

/**
 * ByteArray
 * These are some accumulated utilities for ByteArray manipulation. The primary purpose
 * of these manipulations is to transfer the representations between integer formats
 * and big-endian or network order byte arrays, for communications.
 * 
 * @author Tom Cowin <tcowin@ai.uwf.edu>
 * @version 1.0
 * $Revision: 1.16 $
 */
public class ByteArray
{
    /**
     * Convert a short integer to a 2 byte array in big-endian byte order
     *
     * @param i the short to be changed to byte array
     *
     * @return the byte[] with the same value as the input Short
     */
    public static byte[] shortToByteArray (short i) 
    {
        // Fill byte array using big-endian byte ordering
		byte[] bytes = new byte[2];
		
        bytes[0] = (byte) ((i & (short) 0xFF00) >>> 8);
        bytes[1] = (byte) (i & (short) 0x00FF);

        return bytes;
    }


    /**
     * Convert a byte array to a short integer, byte array is assumed to be big-endian
     *
     * @param bytes the byte array to be read
     *
     * @return the short with the same value as the input byte[]
     */
    public static short byteArrayToShort (byte[] bytes) 
    {
        return byteArrayToShort (bytes, 0);
    }

    
    /**
     * Convert a byte array to a short integer, byte array is assumed to be big-endian
     *
     * @param bytes the byte array to be read
     * @param offset the offset
     *
     * @return the short with the same value as the input byte[]
     */
    public static short byteArrayToShort (byte[] bytes, int offset) 
    {
        // Byte array should be using big-endian(network) byte ordering where MSB comes first
        short i;

        i = (short) ((bytes[offset] << 8) & (short) 0xFF00);
        i |= (short) (bytes[offset + 1] & (short) 0x00FF);

        return i;
    }


    /**
     * Change an Integer to a byte array with length of four in big endian or network byte order
     *
     * @param i  the Integer to be changed to byte array
     *
     * @return the byte[] with the same value as the input Interger
     */
    public static byte[] intToByteArray (int i)
    {
        byte newBytearray[] = new byte[4];
        
        newBytearray[3] = new Integer (i & 0x000000FF).byteValue();
        newBytearray[2] = new Integer ((i & 0x0000FF00) >> 8).byteValue();
        newBytearray[1] = new Integer ((i & 0x00FF0000) >> 16).byteValue();
        newBytearray[0] = new Integer ((i & 0xFF000000) >> 24).byteValue();
        return newBytearray;
    }

    /**
     * Change a int into a byte array
     *
     * @param value the Integer to be changed to byte array
     * @param bytes the byte[] which will contains the int
     */
    public static void intToByteArray (int value, byte[] bytes)
    {
        intToByteArray (value, bytes, 0);
    }

    public static void intToByteArray (int value, byte[] bytes, int off)
    {
        bytes[off] = (byte) ((value >> 24) & 0xFF);
        bytes[off+1] = (byte) ((value >> 16) & 0xFF);
        bytes[off+2] = (byte) ((value >> 8) & 0xFF);
        bytes[off+3] = (byte) (value & 0xFF);
    }

    /**
     * Change a byte array into a int
     *
     * @param bytes byte array to be read
     *
     * @return the int with the same value as the input byte[]
     */
    public static int byteArrayToInt (byte[] bytes)
    {
        return byteArrayToInt (bytes, 0);
    }

    /**
     * Change a byte array with length of four to its integer value. Input byte array is assumed to 
     * be in big endian or network byte order
     *
     * @param bytes     the byte array with the big endian integer
     * @param off       the offset into the byte array where the value should be read
     *
     * @return          the integer value contained in the byte array
     */
    public static int byteArrayToInt (byte[] bytes, int off)
    {
        int num = 0;
        for(int i = 0; i < 4; i++) {
	        num = (num << 8) | (bytes[off+i] & 0xFF);
	    }
        return num;
    }

    /**
    * Convert <code>length</code> bytes from an array of bytes starting 
    * at position <code>start</code> to long
    *
    * @param in Array of bytes to convert
    * @return the <code>long</code> representation of the <code>byte[]</code> content
    */
   public static long byteArrayToLong (byte[] in)
   {
       return byteArrayToLong (in, 0, in.length - 1);
   }
    
    /**
     * Convert <code>length</code> bytes from an array of bytes starting 
     * at position <code>start</code> to long
     *
     * @param in Array of bytes to convert
     * @param start First index in <code>in</code> to read
     * @param length Number of bytes to read
     * @return the <code>long</code> representation of the <code>byte[]</code> content
     */
    public static long byteArrayToLong (byte[] in, int start, int length)
    {
        long value = 0;

        for (int i = start; i < (start + length); i++) {
            // move each byte (length-pos-1)*8 bits to the left and add them
            value += (long) ((in[i] & 0xFF) << ((length - i + start - 1) * 8));
        }
        return value;
    }

    /**
     * Convert long value to array of bytes. 
     *
     * @param in Long value to convert
     * @param len Length of resulting byte array. <code>-1</code> for minimum
     *        length needed.
     * @return Newly created array of bytes with enough fields to hold input
     *          First entry contains the MSB.
     */
    public static byte[] longToByteArray (long in, int len)
    {
        if (len == -1) {
            // get length of result array (log2 n bits => log2 n / 8 Bytes)
            len = (int)(Math.ceil (Math.ceil (Math.log(in) / Math.log (2)) / 8));
        }

        byte[] res = new byte[len];
 
        long act = in;
        for (int i = 0; i < len; i++) {
            // Move now handled byte to the right
            res[i] = (byte)(act >> ((len - i - 1) * 8));
            // And remove all bytes to the left
            res[i] = (byte) (res[i] & 0xff);
        }

        return res;
    }

    /**
     * Store a Java string into a byte array and if necessary, pad the bytes with NULL characters.
     * Note that this method only supports strings that do not use any characters larger than 8-bits.
     * 
     * @param str       the string to be stored
     * @param buf       the byte array into which the string should be stored
     * @param off       the starting offset into the byte array
     * @param len       the number of bytes that the string should occupy in the byte array; if
     *                      necessary, null characters will be used for padding
     */
    public static void stringToByteArray (String str, byte[] buf, int off, int len)
    {
        int strLen = str.length();
        if (len > 0) {
            // Assume that the string should not exceed the specified length and
            // if necessary should be padded null characters
            if (strLen > len) {
                throw new IllegalArgumentException ("length of string is " + strLen + ", which exceeds len argument of " + len);
            }
        }
        for (int i = 0; i < strLen; i++) {
            char ch = str.charAt (i);
            if (ch > 255) {
                throw new IllegalArgumentException ("string '" + str + "' contains characters beyond the 8-bit ASCII character set");
            }
            buf[off+i] = (byte) ch;
        }
        if (len > strLen) {
            for (int i = strLen; i < len; i++) {
                buf[off+i] = 0;
            }
        }
    }

    /**
     * Says if the two byte arrays are equal
     *
     * @param ba1 first byte array
     * @param ba2 second byte array
     *
     * @return true if the two byte arrays are equal
     */
    public static boolean areEqual (byte[] ba1, byte[] ba2)
    {
        if (ba1.length != ba2.length) {
            return false;
        }
        for (int j = 0; j < ba1.length; j++) {
            if (ba1[j] != ba2[j]) {
                return false;
            }
        }
        return true;
    }
 
    /**
     * Get a string out of a byte array. Note that this method only supports strings
     * that do not use any characters larger than 8-bits.
     * 
     * @param buf       the byte array that contains the string
     * @param off       the starting offset into the byte array
     * @param maxLen    the maximum length of the string; the string will be
     *                  terminated after either a null character or after maxLen
     *                  characters; if maxLen is <= 0, the string will be as long
     *                  as necessary (until a null character is found)
     * 
     * @return          the extracted string
     */
    public static String byteArrayToString (byte[] buf, int off, int maxLen)
    {
        int strLen = 0;

        if (maxLen <= 0) {
            // Count characters until a null character is found
            while (true) {
                if (buf [off+strLen] == 0) {
                    break;
                }
                strLen++;
            }
        }
        else {
            // Count characters until a null character is found or maxLen is reached
            while (strLen < maxLen) {
                if (buf [off+strLen] == 0) {
                    break;
                }
                strLen++;
            }
        }

        return new String (buf, off, strLen);
    }

    /**
     * Prints to screen the byte array as hexadecimal columns
     *
     * @param bytes input byte array
     */
    public static void printByteArrayAsHexColumns (byte[] bytes) 
    {
        if (bytes.length == 0) {
            System.out.println ("Byte array is null!\n");
        }
        else {
            for (int i = 0; i != bytes.length; i++) {
                if (((i % 16) == 0) && (i != 0))
                     System.out.print("\n");
                int h = (((int) bytes[i]) & 0xFF);
                System.out.print (Integer.toHexString(h) + " ");
            }
            System.out.println ("");
        }
    }

    /**
     * Prints to screen the byte array as hexadecimal
     *
     * @param bytes input byte array
     */
    public static void printByteArrayAsHex (byte[] bytes) 
    {
        if (bytes.length == 0) {
            System.out.println ("Byte array is null!\n");
        }
        else {
            for (int i = 0; i != bytes.length; i++) {
                /* if (((i % 16) == 0) && (i != 0))
                     System.out.print("\n");*/
                int h = (((int) bytes[i]) & 0xFF);
                System.out.println ("byteArray[" + i + "]: 0x" + Integer.toHexString(h));
            }
            System.out.println ("");
        }
    }
  
    /***
     * Prints to screen byte array as character string
     *
     * @param bytes input byte array
     */
    public static void printByteArrayAsChar (byte[] bytes) 
    {
        if (bytes.length == 0) {
            System.out.println ("Byte array is null!\n");
        }
        else {
            for (int i = 0; i != bytes.length; i++) {
                System.out.println ("byteArray[" + i + "]: " + (char) bytes[i]);
            }
            System.out.println ("");
        }
    }
    
    /***
     * Prints to screen byte array
     *
     * @param bytes input byte array
     */
    public static void printByteArray(byte[] bytes) 
    {
        if (bytes.length == 0) {
            System.out.println("Byte array is null!\n");
        }
        else {
            for (int i = 0; i != bytes.length; i++) {
                if (((i % 16) == 0) && (i != 0)) {
                    System.out.print("\n");
                }
                System.out.print(bytes[i] + "\t");
            }
            System.out.println("\n");
        }
    }

}
